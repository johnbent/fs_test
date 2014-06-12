#include "iod_api.h"
#include "plfs.h"
#include "stdio.h"
#include "errno.h"
#include <string.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

// some definitions to make the code more readable
#define IOD_SYNCHRONOUS NULL
#define NO_TRANSACTION  0
#define NO_STRUCT       NULL
#define NO_HINTS        NULL
#define NONAME          NULL

#ifndef offsetof
# define offsetof(typ, memb)      ((long)((char *)&(((typ *)0)->memb)))
#endif

int num_elmts = 24*1024*1024;

static const char *
loc_str(iod_location_t loc_type) {
    switch (loc_type) {
    case IOD_LOC_BB: return "ION ";
    case IOD_LOC_CENTRAL: return "DAOS";
    default: return "WTF?";
    }
}

static void
print_iod_obj_map(iod_obj_map_t *obj_map)
{
    int i;
    uint32_t u;

    printf("MAP: oid: %"PRIx64"   type: %d\n", obj_map->oid, obj_map->type);
    printf("Burst Buffers %d:\n", obj_map->n_bb_loc);
    for (i = 0; i < obj_map->n_bb_loc ; i++) {
        iod_bb_loc_info_t *info = obj_map->bb_loc_infos[i];
        int j;

        printf("\tShadow path: %s  nrank = %d:  ", info->shadow_path, info->nrank);
        for(j=0 ; j<info->nrank; j++)
            printf("%d ", info->direct_ranks[j]);
        printf("\n");
    }
    printf("DAOS Shards %d:\n", obj_map->n_central_loc);
    for (i = 0; i < obj_map->n_central_loc ; i++) {
        iod_central_loc_info_t *info = obj_map->central_loc_infos[i];
        int j;

        printf("\tShard ID: %u  nrank = %d:   ", info->shard_id, info->nrank);
        for(j=0 ; j<info->nrank; j++)
            printf("%d ", info->nearest_ranks[j]);
        printf("\n");
    }
    switch(obj_map->type) {
    case IOD_OBJ_BLOB:
        {
            printf("nranges = %d\n", obj_map->u_map.blob_map.n_range);
            for(u = 0; u < obj_map->u_map.blob_map.n_range ; u++) {
                printf(
                        "\toffset: %"PRIu64"  length: %zu  location: %s  index: %d  nearest rank: %d\n",
                        obj_map->u_map.blob_map.blob_range[u].offset, 
                        obj_map->u_map.blob_map.blob_range[u].len,
                        loc_str(obj_map->u_map.blob_map.blob_range[u].loc_type),
                        obj_map->u_map.blob_map.blob_range[u].loc_index,
                        obj_map->u_map.blob_map.blob_range[u].nearest_rank);
            }
            break;
        }
    case IOD_OBJ_ARRAY:
        {
            printf("nranges = %d\n", obj_map->u_map.array_map.n_range);
            for(u = 0; u < obj_map->u_map.array_map.n_range ; u++) {
                printf(
                        "\tlocation: %s  index: %d  nearest rank: %d  ",
                        loc_str((obj_map->u_map.array_map.array_range[u].loc_type)),
                        obj_map->u_map.array_map.array_range[u].loc_index,
                        obj_map->u_map.array_map.array_range[u].nearest_rank);
                printf("start [%"PRIu64"] end [%"PRIu64"]\n",
                        obj_map->u_map.array_map.array_range[u].start_cell[0],
                        obj_map->u_map.array_map.array_range[u].end_cell[0]);
            }
            break;
        }
        break;
    case IOD_OBJ_KV:
    case IOD_OBJ_INVALID:
    case IOD_OBJ_ANY:
    default:
        break;
    }
}

int
main(int argc, char **argv) {
    iod_handle_t coh, wr_oh,rd_oh;
    int mpi_size, mpi_rank;
    int multi_thread_provide;
    MPI_Comm comm  = MPI_COMM_WORLD;
    iod_trans_id_t tid = 0;
    iod_array_struct_t array_struct_set;
    iod_obj_id_t array_id = 0;
    iod_size_t start[1];
    iod_size_t count[1];
    iod_size_t stride[1];
    iod_size_t block[1];
    uint32_t *warray, j;
    char *file_name = argv[1];
    int no_write = atoi(argv[2]);
    iod_hint_list_t *con_open_hint = NULL;
    iod_hint_list_t *obj_create_hint = NULL;
    iod_ret_t ret;

    if(file_name == NULL) {
        printf("need file name\n");
        exit(1);
    }

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &multi_thread_provide);
    if (multi_thread_provide != MPI_THREAD_MULTIPLE) {
        fprintf( stderr, "ERROR: Unable to initialize MPI (MPI_Init).\n");
	return 1;
    }
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    ret = iod_initialize(comm, NULL, mpi_size, mpi_size);
    if (ret != 0) {
        printf("iod_initialize failed rc: %d, exit.\n", ret);
        assert(0);
    }

    IOD_OBJID_SETOWNER_APP(array_id)
    IOD_OBJID_SETTYPE(array_id, IOD_OBJ_ARRAY)

    if(no_write) {
        iod_cont_trans_stat_t *tids = NULL;

        ret = iod_container_open(file_name, NULL, IOD_CONT_RW,
                                 &coh, IOD_SYNCHRONOUS);
        if(ret != 0) {
            printf("iod_container_open failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        iod_query_cont_trans_stat(coh, &tids, NULL);

        printf("latest readable %d; latest durable %d\n", 
               (int)tids->latest_rdable, (int)tids->latest_durable);

        tid = tids->latest_rdable;
        iod_free_cont_trans_stat(coh, tids);

        ret = iod_trans_start(coh, &tid, NULL, 0, IOD_TRANS_R, NULL);
        assert(ret == 0);

        if ((ret = iod_obj_open_read(coh, array_id, tid, NULL, &rd_oh, NULL)) < 0) {
            printf("iod_obj_open_read failed %d (%s).\n", ret, strerror(-ret));
            assert(0);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if(0 == mpi_rank) {
            iod_obj_map_t *obj_map = NULL;

            ret = iod_obj_query_map(rd_oh, tid, &obj_map, NULL);
            if (ret != 0)
                assert(0);

            printf("**************** READ ONLY **************\n");
            print_iod_obj_map(obj_map);
            printf("**************** END *****************\n");

            ret = iod_obj_free_map(rd_oh, obj_map);
            if(ret < 0)
                assert(0);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
        if(ret != 0) {
            printf("iod_trans_finish failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        ret = iod_obj_close(rd_oh, NULL, NULL);
        if(ret != 0) {
            printf("iod_obj_close wr_oh failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }
        MPI_Barrier(MPI_COMM_WORLD);

        ret = iod_container_close(coh, NULL, IOD_SYNCHRONOUS);
        if(ret != 0) {
            printf("iod_container_close failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        ret = iod_finalize(NULL);
        if(ret != 0) {
            assert(0);
        }

        MPI_Finalize();
        return 0;
    }

    ret = iod_container_open(file_name, NULL, IOD_CONT_CREATE|IOD_CONT_RW,
                             &coh, IOD_SYNCHRONOUS);
    if(ret != 0) {
	printf("iod_container_open failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    tid = 0;
    /************ TR 0 *****************/
    ret = iod_trans_start(coh, &tid, NULL, mpi_size, IOD_TRANS_W, NULL);
    if(ret != 0) {
	printf("iod_trans_start failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
    if(ret != 0) {
	printf("iod_trans_finish failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    warray = malloc(sizeof(uint32_t)*num_elmts);
    for(j=0 ; j<num_elmts ; j++)
        warray[j] = j;

    start[0]  = mpi_rank * num_elmts;
    count[0]  = 1;
    stride[0] = num_elmts;
    block[0]  = num_elmts;

    array_struct_set.cell_size = sizeof(int32_t);
    array_struct_set.num_dims  = 1;
    array_struct_set.current_dims = malloc(sizeof(iod_size_t) *
                                           array_struct_set.num_dims);
    array_struct_set.current_dims[0] = num_elmts * mpi_size;
    array_struct_set.chunk_dims      = NULL;
    array_struct_set.firstdim_max    = IOD_DIMLEN_UNLIMITED;


    {
        iod_array_io_t    array_io;
        iod_ret_t         array_io_ret;
        iod_mem_desc_t    *mem_desc;
        iod_hyperslab_t slab = {start, count, stride, block};

        tid = 1;

        ret = iod_trans_start(coh, &tid, NULL, mpi_size, IOD_TRANS_W, NULL);
        assert(ret == 0);

        if(0 == mpi_rank) {
            ret = iod_obj_create(coh, tid, NULL/*obj_create_hint*/, IOD_OBJ_ARRAY, NULL, 
                                 &array_struct_set,
                                 &array_id, NULL);
            if(ret != 0) {
                printf("iod_obj_create failed, ret: %d (%s).\n",
                       ret, strerror(-ret));
                assert(0);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        mem_desc = (iod_mem_desc_t *)malloc(sizeof(iod_mem_desc_t) + 
                                            sizeof(iod_mem_frag_t));

        /* write to Array */
        if ((ret = iod_obj_open_write(coh, array_id, tid, NULL, &wr_oh, NULL)) < 0) {
            printf("iod_obj_open_write failed %d (%s).\n", ret, strerror(-ret));
            assert(0);
        }

        mem_desc->nfrag = 1;
        mem_desc->frag[0].addr = &warray[0];
        mem_desc->frag[0].len  = num_elmts * sizeof(uint32_t);

        ret = iod_array_write(wr_oh, tid, NULL, mem_desc, &slab, NULL, NULL);
        if(ret != 0) {
            printf("iod_obj_write failed %d (%s).\n", ret, strerror(-ret));
            assert(0);
        }

        if(mem_desc) {
            free(mem_desc);
            mem_desc = NULL;
        }

        ret = iod_obj_close(wr_oh, NULL, NULL);
        if(ret != 0) {
            printf("iod_obj_close wr_oh failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
        if(ret != 0) {
            printf("iod_trans_finish failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        MPI_Barrier (MPI_COMM_WORLD);
    }

    /************ END TR 1 *****************/

    MPI_Barrier(MPI_COMM_WORLD);

    /************ TR 1 *****************/
    ret = iod_trans_start(coh, &tid, NULL, mpi_size, IOD_TRANS_R, NULL);
    if(ret != 0) {
	printf("iod_trans_start failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    if ((ret = iod_obj_open_read(coh, array_id, tid, NULL, &rd_oh, NULL)) < 0) {
        printf("iod_obj_open_read failed %d (%s).\n", ret, strerror(-ret));
        assert(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(0 == mpi_rank) {
        iod_obj_map_t *obj_map = NULL;

        ret = iod_obj_query_map(rd_oh, tid, &obj_map, NULL);
        if (ret != 0)
            assert(0);

        printf("****************Before Persist **************\n");
        print_iod_obj_map(obj_map);
        printf("**************** END *****************\n");
        ret = iod_obj_free_map(rd_oh, obj_map);
        if(ret < 0)
            assert(0);
    }

    if(0 == mpi_rank){
        ret = iod_trans_persist(coh, tid, NULL, NULL);
        if(ret != 0) {
            printf("iod_trans_persist failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
        }

        if(0 == mpi_rank) {
            iod_obj_map_t *obj_map = NULL;
            ret = iod_obj_query_map(rd_oh, tid, &obj_map, NULL);
            if (ret != 0)
                assert(0);
            printf("**************** After Persist Before Purge **************\n");
            print_iod_obj_map(obj_map);
            printf("**************** END *****************\n");
            ret = iod_obj_free_map(rd_oh, obj_map);
            if(ret < 0)
                assert(0);
        }

	ret = iod_obj_purge(rd_oh, tid, NULL, NULL);
	if(ret != 0) {
	  printf("purge failed, ret: %d (%s).\n",
		 ret, strerror(-ret));
	  assert(0);
	}

        if(0 == mpi_rank) {
            iod_obj_map_t *obj_map = NULL;
            ret = iod_obj_query_map(rd_oh, tid, &obj_map, NULL);
            if (ret != 0)
                assert(0);
            printf("**************** After Purge **************\n");
            print_iod_obj_map(obj_map);
            printf("**************** END *****************\n");
            ret = iod_obj_free_map(rd_oh, obj_map);
            if(ret < 0)
                assert(0);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(0 == mpi_rank){
        iod_trans_id_t replica_id;
        iod_layout_t *layout = NULL;
        iod_hyperslab_t slab = {start, count, stride, block};

        layout = (iod_layout_t *)malloc(sizeof(iod_layout_t));

        layout->loc = IOD_LOC_BB;
        layout->type = IOD_LAYOUT_LOGGED;
        layout->dims_seq = NULL;
        layout->target_start = 1;
        layout->target_num = 1;
        layout->stripe_size = num_elmts;

        ret = iod_obj_fetch(rd_oh, tid, NULL, NULL, layout, &replica_id, NULL);
	if(ret != 0) {
            printf("fetch failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
            assert(0);
	}

        free(layout);

        printf("prefetched replica %"PRIx64"\n", replica_id);

        if(0 == mpi_rank) {
            iod_obj_map_t *obj_map = NULL;
            ret = iod_obj_query_map(rd_oh, replica_id, &obj_map, NULL);
            if (ret != 0)
                assert(0);
            printf("**************** After Prefetch **************\n");
            print_iod_obj_map(obj_map);
            printf("**************** END *****************\n");
            ret = iod_obj_free_map(rd_oh, obj_map);
            if(ret < 0)
                assert(0);
        }

	ret = iod_obj_purge(rd_oh, replica_id, NULL, NULL);
	if(ret != 0) {
	  printf("purge failed, ret: %d (%s).\n",
		 ret, strerror(-ret));
	  assert(0);
	}

        if(0 == mpi_rank) {
            iod_obj_map_t *obj_map = NULL;
            ret = iod_obj_query_map(rd_oh, tid, &obj_map, NULL);
            if (ret != 0)
                assert(0);
            printf("**************** After Purge **************\n");
            print_iod_obj_map(obj_map);
            printf("**************** END *****************\n");
            ret = iod_obj_free_map(rd_oh, obj_map);
            if(ret < 0)
                assert(0);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    ret = iod_obj_close(rd_oh, NULL, NULL);
    if(ret != 0) {
        printf("iod_obj_close wr_oh failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
    if(ret != 0) {
	printf("iod_trans_finish failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    ret = iod_container_close(coh, NULL, IOD_SYNCHRONOUS);
    if(ret != 0) {
	printf("iod_container_close failed, ret: %d (%s).\n",
               ret, strerror(-ret));
        assert(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    ret = iod_finalize(NULL);
    if(ret != 0) {
        assert(0);
    }

    MPI_Finalize();
    return 0;
}
