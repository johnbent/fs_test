#include "iod_api.h"
#include "stdio.h"
#include "errno.h"
#include "plfs.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <limits.h>

#include <ctime>
using namespace std;

#ifndef offsetof
# define offsetof(typ, memb)      ((long)((char *)&(((typ *)0)->memb)))
#endif

#define STARTTIME(x) x = clock();
#define ENDTIME(x,y) y = ( clock() - x ) / (double) CLOCKS_PER_SEC;	

#define IOD_SYNCHRONOUS NULL

int num_steps = 1;
int persist_rate = 1;
char *cname = NULL;
int num_elmts = 1024*128;
int cell_size = 1024;
int enable_cksums = 1;
int silence = 0;

#define OUTPUT_BUFSZ 4096
char final_output[OUTPUT_BUFSZ];

class Average {
	public:
		Average() { quantity = sum = 0; }
		double mean() { return (double)sum/quantity; }
		void add_value(double d) { sum += d; quantity++; }
	private:
		unsigned quantity;
		double sum;
};

Average persist_time;
Average commit_time;

static void print_usage()
{
    printf("Usage: \n");
    printf("-f container_name (default = %s)\n", cname);
    printf("-p number of elements in array per rank (default = %d)\n",num_elmts);
    printf("-s number of steps/transactions/arrays (default = %d)\n", num_steps);
    printf("-r persist rate (default = %d)\n", persist_rate);
	printf("-c cell_size (default = %d)\n", cell_size);
	printf("-x [0|1] (default = %d) # enable iod cksums\n", enable_cksums);
	printf("-S # silent mode, run with less output\n");
}

static void
print_time(int rank, const char *c, double d) {
	if (!rank && !silence) {
		printf("Operation %s took %.4f seconds\n", c, d);
	}
}

static void
add_time(int rank, const char *c, double d) {
	print_time(rank,c,d);
	snprintf((char*)&final_output[strlen(final_output)], OUTPUT_BUFSZ - strlen(final_output),
		" | %s %.4f", c, d );
}

static void
add_persist(int rank, const char *c, double d) {
	print_time(rank,c,d);
	if ( !rank ) {
		persist_time.add_value(d);
	}
}

static void
add_commit(int rank, const char *c, double d) {
	print_time(rank,c,d);
	if ( !rank ) {
		commit_time.add_value(d);
	}

}

static int
query_container(int rank, iod_handle_t coh) {
	int ret = 0;
    iod_cont_trans_stat_t *trans_stat = NULL;
	if (!rank) {
		/* let's query the transaction status of the container */
		ret = iod_query_cont_trans_stat(coh,&trans_stat,IOD_SYNCHRONOUS);
		assert(ret==0);
		if ( !silence ) {
			printf("iod_query_cont_trans_stat rc: %d.\n", ret);
			printf("trans_stat->lowest_durable: %lli.\n",
				   trans_stat->lowest_durable);
			printf("trans_stat->latest_rdable: %lli.\n",
				   trans_stat->latest_rdable);
			printf("trans_stat->latest_wrting: %lli.\n",
			   trans_stat->latest_wrting);
		}

		/* let's list the objects in the latest durable */
		int num_objs=INT_MAX;
		ret = iod_container_list_obj(coh, 0, IOD_OBJ_ARRAY, 0, &num_objs,
                         NULL, NULL, NULL, NULL);
		assert(ret==0);
        if (!silence) printf("%s has %d objs at tid 0\n", cname, num_objs);


		int objs_per_call = 10;
		iod_obj_id_t *oid_list = 
			(iod_obj_id_t*)malloc(objs_per_call*sizeof(iod_obj_id_t*));
		iod_obj_type_t *type_list = 
			(iod_obj_type_t*)malloc(objs_per_call*sizeof(iod_obj_type_t*));	
		assert(oid_list && type_list);
		int obj_offset=0;
		do {
			num_objs=objs_per_call;
			ret = iod_container_list_obj(coh, trans_stat->latest_rdable,
				IOD_OBJ_ANY, obj_offset, &num_objs, oid_list, type_list,
				NULL, NULL);
			assert(ret==0);
			if (!silence) {
				printf("%s has %d objs at tid=%d\n", cname, num_objs,
					trans_stat->latest_rdable);
				for(int i=0; i<num_objs; i++){
					printf("Object %d : OID %lli type %d\n", i+obj_offset, oid_list[i], type_list[i]);
				}
			}
			obj_offset += num_objs;
		} while (num_objs==objs_per_call);
		free(oid_list);
		free(type_list);
		ret = iod_free_cont_trans_stat(coh, trans_stat);
	}
	return ret;
}

static int
close_container(iod_handle_t coh) {
    int ret = iod_container_close(coh, NULL, NULL);
    if(ret != 0) {
		fprintf(stderr,"iod_container_close failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }
	return ret;
}

static int
parse_options(int argc, char** argv, int ranks) 
{
    int c;
    extern char *optarg;

	cname = (char*)malloc(sizeof(char)*1024);
	snprintf(cname, 1024, "%s.%d", getenv("USER"), (int)time(NULL)); 

    while ((c = getopt (argc, argv, "Sp:s:r:f:c:x:")) != -1) {
        switch (c) {
		case 'S':
			silence = 1;
			break;
        case 'p':
            num_elmts = atoi(optarg);
            break;
        case 's':
            num_steps = atoi(optarg);
            break;
        case 'r':
            persist_rate = atoi(optarg);
            break;
        case 'f':
			free(cname);
            cname = strdup(optarg);
            break;
		case 'c': 
			cell_size = atoi(optarg);
			break;
		case 'x':
			enable_cksums = atoi(optarg);
			break;
        default:
            print_usage();
			exit(0);
        }
    }

	snprintf(final_output, OUTPUT_BUFSZ, "%s | ranks %d | file %s | cells %d | cellsz %d | steps %d | persist %d | cksums %d",
		argv[0], ranks, cname, num_elmts, cell_size, num_steps, persist_rate, enable_cksums );
    return( 0 );
}

int
main(int argc, char **argv) {
    // **** STEP 1: Create an H5File at /scratch/foo **********************
    iod_handle_t coh, wr_oh,rd_oh;
    int mpi_size, mpi_rank;
    int multi_thread_provide;
    MPI_Comm comm  = MPI_COMM_WORLD;
    iod_trans_id_t tid, i;
    iod_array_struct_t array_struct_set;
    iod_obj_id_t array_id = 0;
    iod_size_t start[1];
    iod_size_t count[1];
    iod_size_t stride[1];
    iod_size_t block[1];
    uint32_t *warray, j;
    iod_hint_list_t *con_open_hint = NULL;
    iod_hint_list_t *obj_create_hint = NULL;
    iod_ret_t ret;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &multi_thread_provide);
    if (multi_thread_provide != MPI_THREAD_MULTIPLE) {
        fprintf( stderr, "ERROR: Unable to initialize MPI (MPI_Init).\n");
	return 1;
    }

    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    if (parse_options(argc, argv, mpi_size) != 0 ) {
        exit( 1 );
    }

    if(NULL == cname) {
        print_usage();
        fprintf(stderr, "Invalid file name\n");
        exit(1);
    }

    if(!mpi_rank && !silence) 
        printf("running test with %d ranks, %d steps/arrays, %d elements of size %d per array per rank\n",
               mpi_size, num_steps, num_elmts, cell_size);

	clock_t starttime;
	double duration;
	STARTTIME(starttime);
    ret = iod_initialize(comm, NULL, mpi_size, mpi_size);
    if (ret != 0) {
        printf("iod_initialize failed rc: %d, exit.\n", ret);
        assert(0);
    }
	ENDTIME(starttime,duration);
	add_time(mpi_rank, "iod_init", duration);

    /* scratch pad integrity in the container */
    con_open_hint = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
    con_open_hint->num_hint = 1;
    con_open_hint->hint[0].key = (char*)"iod_hint_co_scratch_cksum";

	if (enable_cksums) {
		obj_create_hint = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
		obj_create_hint->num_hint = 1;
		obj_create_hint->hint[0].key = (char*)"iod_hint_obj_enable_cksum";
	} else {
		obj_create_hint = NULL;
	}

	STARTTIME(starttime);
    ret = iod_container_open(cname, con_open_hint, IOD_CONT_CREATE|IOD_CONT_RW,
                             &coh, NULL);
	ENDTIME(starttime,duration);
	add_time(mpi_rank, "iod_container_open", duration);
    if(ret != 0) {
        fprintf(stderr, "iod_container_open w create failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

	/* skipping the weird tid=0 */
    tid = 0;
    ret = iod_trans_start(coh, &tid, NULL, mpi_size, IOD_TRANS_W, NULL);
    if(ret != 0) {
        fprintf(stderr,"iod_trans_start failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }
    ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
    if(ret != 0) {
        fprintf(stderr,"iod_tr finish failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

	/* setting up the write buffer */
    warray = (uint32_t*)malloc(cell_size*num_elmts);
	if ( ! warray ) {
		fprintf( stderr, "malloc: %s\n", strerror(errno));
		return(-1);
	}
    for(j=0 ; j<num_elmts ; j++)
        warray[j] = j;

	/* setting up the iod array structure */
    start[0]  = mpi_rank * num_elmts;
    count[0]  = 1;
    stride[0] = num_elmts;
    block[0]  = num_elmts;

    array_struct_set.cell_size = cell_size; 
    array_struct_set.num_dims  = 1;
    array_struct_set.current_dims = (iod_size_t*)malloc(sizeof(iod_size_t) *
                                           array_struct_set.num_dims);
    array_struct_set.current_dims[0] = num_elmts * mpi_size;
    array_struct_set.chunk_dims      = NULL;
    array_struct_set.firstdim_max    = IOD_DIMLEN_UNLIMITED;

    for(i=0 ; i<num_steps ; i++) {
        iod_array_io_t    array_io;
        iod_ret_t         array_io_ret;
        iod_mem_desc_t    *mem_desc;
        iod_hyperslab_t slab = {start, count, stride, block};

        tid = i+1;

        if(!mpi_rank) {
			if (!silence) printf("starting Step %d (transaction %d)\n", (int)i , (int)tid);
			else printf("t%d..",tid);
		}

		STARTTIME(starttime);
        ret = iod_trans_start(coh, &tid, NULL, mpi_size, IOD_TRANS_W, NULL);
        assert(ret == 0);

        IOD_OBJID_SETOWNER_APP(array_id)
        IOD_OBJID_SETTYPE(array_id, IOD_OBJ_ARRAY)

        if(0 == mpi_rank) {
            ret = iod_obj_create(coh, tid, obj_create_hint, IOD_OBJ_ARRAY, NULL, 
                                 &array_struct_set,
                                 &array_id, NULL);
            if(ret != 0) {
                fprintf(stderr,"iod_obj_create failed, ret: %d (%s).\n",
                       ret, strerror(-ret));
                assert(0);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        mem_desc = (iod_mem_desc_t *)malloc(sizeof(iod_mem_desc_t) + 
                                            sizeof(iod_mem_frag_t));

        /* write to Array */
        if ((ret = iod_obj_open_write(coh, array_id, tid, NULL, &wr_oh, NULL)) < 0) {
            fprintf(stderr,"iod_obj_open_write failed %d (%s).\n", ret, strerror(-ret));
            assert(0);
        }

        mem_desc->nfrag = 1;
        mem_desc->frag[0].addr = &warray[0];
        mem_desc->frag[0].len  = num_elmts * cell_size; 

        ret = iod_array_write(wr_oh, tid, NULL, mem_desc, &slab, NULL, NULL);
        if(ret != 0) {
            fprintf(stderr,"iod_obj_write failed %d (%s).\n", ret, strerror(-ret));
            assert(0);
        }

        if(mem_desc) {
            free(mem_desc);
            mem_desc = NULL;
        }

        ret = iod_obj_close(wr_oh, NULL, NULL);
        if(ret != 0) {
            fprintf(stderr,"iod_obj_close wr_oh failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
        if(ret != 0) {
            fprintf(stderr,"iod_trans_finish failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        MPI_Barrier (MPI_COMM_WORLD);
		ENDTIME(starttime,duration);
		char t_description[1024];
		snprintf(t_description, 1024, "commit=%d", tid);
		add_commit(mpi_rank, t_description, duration);

        if(mpi_rank == 0 && (i+1)%persist_rate == 0) {
            if (!silence) printf("Persisting transaction %d\n", (int) tid);
			else printf("p%d..", tid);
			STARTTIME(starttime);
            ret = iod_trans_persist(coh, tid, NULL, NULL);
			ENDTIME(starttime,duration);
			snprintf(t_description, 1024, "persist=%d", tid);
			add_persist(mpi_rank, t_description, duration);
            if(ret != 0) {
                fprintf(stderr,"iod_trans_persist failed, ret: %d (%s).\n",
                       ret, strerror(-ret));
                assert(0);
            }
        }
        array_id ++;
    }

    MPI_Barrier(MPI_COMM_WORLD);

	if (silence) printf("\n");

	STARTTIME(starttime);
	query_container(mpi_rank,coh);
	ENDTIME(starttime,duration);	
	add_time(mpi_rank, "iod_cont_query", duration);

    MPI_Barrier(MPI_COMM_WORLD);

	STARTTIME(starttime);
	close_container(coh);
	ENDTIME(starttime,duration);
	add_time(mpi_rank, "iod_cont_close", duration);

    MPI_Barrier(comm);

	STARTTIME(starttime);
    ret = iod_finalize(NULL);
    if(ret != 0) {
        assert(0);
    }
	ENDTIME(starttime,duration);
	add_time(mpi_rank, "iod_fini", duration);

    free(array_struct_set.current_dims);
    free(warray);

    if(con_open_hint) {
        free(con_open_hint);
        con_open_hint = NULL;
    }
    if(obj_create_hint) {
        free(obj_create_hint);
        obj_create_hint = NULL;
    }

    MPI_Finalize();

	add_time(mpi_rank, "ave_commit", commit_time.mean()); 
	add_time(mpi_rank, "ave_persist", persist_time.mean()); 
	if (!mpi_rank){
		 printf("######\n%s\n#####\n", final_output);

		FILE *fp = fopen("/scratch/iod/output/array_test", "a");
		if (fp) {
			fwrite(final_output, sizeof(char), strlen(final_output), fp);
			fwrite("\n", sizeof(char), 1, fp);
			fclose(fp);
		}
	}
    return 0;
}
