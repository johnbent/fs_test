#ifdef HAS_IOD

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "iod.h"
#include "plfs.h"

#define IDEBUG(rank, format, ...)                     \
do {                                    \
    int _rank = (rank);                         \
                                    \
    if (_rank == 0) {                          \
        fprintf(stdout, "%.2f IOD DEBUG (%s:%d): %d: : "       \
            format"\n", MPI_Wtime(), \
            __FILE__, __LINE__, rank,       \
            ##__VA_ARGS__);         \
        fflush(stdout);                     \
    }                                   \
} while (0);

#define IOD_RETURN_ON_ERROR(X,Y) { \
	if (Y != 0 ) { \
		IOD_PRINT_ERR(X,Y);\
		return Y; \
	} \
}

#define IOD_PRINT_ERR(X,Y) { \
	fprintf(stderr,"IOD Error in %s:%d on %s: %s\n", \
		__FILE__, __LINE__, X, strerror(-Y));\
}

int
iod_getattr( iod_state_t *s,struct stat *sbuf) {
	return ENOSYS;
}

int
iod_close( iod_state_t *s) {
	iod_ret_t ret;

	/* close the object handle */
	ret = iod_obj_close(s->oh, NULL, NULL);
	IOD_RETURN_ON_ERROR("iod_obj_close",ret);

	/* finish the transaction */
	if (s->myrank == 0) {
		ret = iod_trans_finish(s->coh, s->tid, NULL, 0, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_finish",ret);
		IDEBUG(s->myrank,"iod_trans_finish %d: success", s->tid);
	}
	MPI_Barrier(s->mcom);
	
	return ret;
}

int
iod_persist(iod_state_t *s) {

	iod_ret_t ret = 0;
	MPI_Barrier(s->mcom);
	if (s->myrank == 0) {
		ret = iod_trans_start(s->coh, &(s->tid), NULL, 0, IOD_TRANS_R, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_start", ret);

		IDEBUG(s->myrank,"Persist on TR %d", s->tid);
		ret = iod_trans_persist(s->coh, s->tid, NULL, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_persist", ret);

		ret = iod_trans_finish(s->coh, s->tid, NULL, 0, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_finish", ret);
	}
	MPI_Barrier(s->mcom);
	return ret;
}

int
iod_unlink(iod_state_t *s) {
/*

    MPI_Barrier(MPI_COMM_WORLD);

#if CONTAINER_UNLINK
    if (mpi_rank == 0) {
        iod_event_init(ev, eqh);
        ret = iod_container_unlink(cname, 1, ev);
        if(ret == 0) {
            iod_eq_poll(eqh, 0, IOD_EQ_WAIT, 1, &ev_o);
            assert(ev_o == ev);
	    if (ev->ev_rc != 0)
		printf("iod_container_unlink failed, ret: %d (%s).\n",
		       ev->ev_rc, strerror(-ev->ev_rc));
        } else {
	    printf("iod_container_unlink failed, ret: %d (%s).\n",
                   ret, strerror(-ret));
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
#endif
 */
	return ENOSYS;
}

int
iod_trunc(iod_state_t *s) {
	return ENOSYS;
}

int
iod_fini(iod_state_t *s) {
	iod_hint_list_t *hints = NULL;

	/* close the container here */
    iod_ret_t ret = iod_container_close(s->coh, NULL, NULL);
	IOD_RETURN_ON_ERROR("iod_container_close",ret);
	MPI_Barrier(s->mcom);

    ret = iod_finalize(hints);
	IOD_RETURN_ON_ERROR("iod_finalize",ret);

	/* cleanup memory alloc'd in iod_init */
	if (s->mem_desc) free(s->mem_desc); 
	if (s->io_desc) free(s->io_desc);
	if (s->cksum) free (s->cksum);

	return (int)ret;
}

int
iod_sync( iod_state_t *s) {
	return ENOSYS;
}

int
iod_init( struct Parameters *p, struct State *s, MPI_Comm comm ) {
        IDEBUG(s->my_rank,"%d: Init iod with %d ranks",s->my_rank,p->num_procs_world);
	s->iod_state.mcom = comm;
	s->iod_state.myrank = s->my_rank;
	int ret = iod_initialize(comm, NULL, 
			p->num_procs_world, p->num_procs_world );

	iod_state_t *I = &(s->iod_state);

	iod_hint_list_t *con_open_hint = NULL;

	/*
	con_open_hint = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
	con_open_hint->num_hint = 1;
	con_open_hint->hint[0].key = "iod_hint_co_scratch_cksum";
	*/

	/* open and create the container here */
	char *target = expand_path( p->tfname, p->test_time, p->num_nn_dirs, s );
        IDEBUG(s->my_rank, "About to open container %s with %d ranks", 
				target, p->num_procs_world );
	ret = iod_container_open(target, con_open_hint, IOD_CONT_CREATE|IOD_CONT_RW,
							 &(I->coh), NULL);
	if ( ret != 0 ) return ret;

	/* skip TID=0 */
	if (!s->my_rank) { 
		IDEBUG(s->my_rank,"About to start tid=0 on container %s with %d ranks", target, p->num_procs_world );
		I->tid = 0;
		int mpi_size = p->num_procs_world;
		ret = iod_trans_start(I->coh, &(I->tid), NULL, 0, IOD_TRANS_W, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_start",ret);
		IDEBUG(s->my_rank,"About to finish tid=0 on container %s with %d ranks", target, p->num_procs_world );
		ret = iod_trans_finish(I->coh, I->tid, NULL, 0, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_finish",ret);
	}

	/* setup the io and memory descriptors used for blob io */
	s->iod_state.mem_desc = 
		malloc(sizeof(iod_mem_desc_t) + sizeof(iod_mem_frag_t));
	s->iod_state.io_desc = 
		malloc(sizeof(iod_blob_iodesc_t) + sizeof(iod_blob_iofrag_t));
	assert(s->iod_state.mem_desc && s->iod_state.io_desc);

	/* setup the checksum used for blob */
	if (s->iod_state.params.checksum) {
            IDEBUG(s->my_rank, "Using checksums");
		s->iod_state.cksum = malloc(sizeof(iod_checksum_t));
		assert(s->iod_state.cksum);

	} else {
            IDEBUG(s->my_rank, "Not using checksums");
		s->iod_state.cksum = NULL;
	}

	return ret;
}

static int
create_obj(iod_handle_t coh, iod_trans_id_t tid, iod_obj_id_t *oid, iod_obj_type_t type, 
		iod_array_struct_t *structure, iod_hint_list_t *hints, int rank) 
{
	IDEBUG(rank,"Creating obj type %d %lli", type, *oid);
	int ret = iod_obj_create(coh, tid, hints, type, NULL, 
			structure, oid, NULL);
	IOD_RETURN_ON_ERROR("iod_obj_create",ret);
	return ret;
}

int
set_checksum(iod_hint_list_t **hints, int checksum) {
	if (checksum) {
		*hints = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
		assert(*hints);
		(*hints)->num_hint = 1;
		(*hints)->hint[0].key = "iod_hint_obj_enable_cksum";
	}
	return 0;
}

int
open_rd(iod_state_t *s, char *filename, MPI_Comm mcom) {
	iod_ret_t ret = 0;

	/* start the read trans */
	if (s->myrank == 0) {
		ret=iod_trans_start(s->coh, &(s->tid),NULL,0,IOD_TRANS_R,NULL);
		IOD_RETURN_ON_ERROR("iod_obj_open_read", ret);
	}
	MPI_Barrier(mcom);

	ret = iod_obj_open_read(s->coh, s->oid, s->tid, NULL, &(s->oh), NULL);
	IOD_RETURN_ON_ERROR("iod_obj_open_read", ret);
	return ret;
}

int
open_wr(iod_state_t *I, char *filename, MPI_Comm mcom) {
	iod_parameters_t *P = &(I->params);
	int ret = 0;

	/* start the write trans */
	I->tid++;
	if (I->myrank == 0) {
		ret = iod_trans_start(I->coh, &(I->tid), NULL, 0, IOD_TRANS_W, NULL);
		IOD_RETURN_ON_ERROR("iod_trans_start", ret);
		IDEBUG(I->myrank,"iod_trans_start %d : success", I->tid );
	}
	MPI_Barrier(mcom);

	/* create the obj */
	/* only rank 0 does create then a barrier */
	/* alternatively everyone does create, most fail with EEXIST, no barrier */
	I->otype = IOD_OBJ_BLOB;
        srand(time(NULL));
	I->oid = rand()%1024; // make a random one to make sure that striping works
	IOD_OBJID_SETOWNER_APP(I->oid)
	IOD_OBJID_SETTYPE(I->oid, IOD_OBJ_BLOB);
	if( !I->myrank ) {
		iod_hint_list_t *hints = NULL;
		set_checksum(&hints,P->checksum);
		ret = create_obj(I->coh, I->tid, &(I->oid), I->otype, NULL, hints,I->myrank);
		if (hints) free(hints);
		if ( ret != 0 ) {
			return ret;
		} else {
			IDEBUG(I->myrank,"iod obj %lli created successfully.",I->oid);
		}
	}
	MPI_Barrier(mcom);

	/* now open the obj */
	ret = iod_obj_open_write(I->coh, I->oid, I->tid, NULL, &(I->oh), NULL);
	IOD_RETURN_ON_ERROR("iod_obj_open_write", ret);
	return ret;	
}

int
setup_blob_io(size_t len, off_t off, char *buf, iod_state_t *s, int rw) { 

	/* where is the IO directed in the object */
	s->io_desc->nfrag = 1;
	s->io_desc->frag[0].len = len;
	s->io_desc->frag[0].offset = off;
	
	/* from whence does the IO come in memory */
	s->mem_desc->nfrag = 1;
	s->mem_desc->frag[0].addr = (void*)buf;
	s->mem_desc->frag[0].len = len;

	/* setup the checksum */
	if (s->params.checksum) {
		if (rw==WRITE_MODE) {
			plfs_error_t pret;
			pret  = plfs_get_checksum(buf,len,(Plfs_checksum*)s->cksum);
			IOD_RETURN_ON_ERROR("plfs_get_checksum", pret);
		} else {
			memset(s->cksum,0,sizeof(*(s->cksum)));
		}
	}
}


int 
iod_write(iod_state_t *I,char *buf,size_t len,off_t off,ssize_t *bytes) {
	iod_ret_t ret;
	iod_hint_list_t *hints = NULL;
	iod_event_t *event = NULL;

	setup_blob_io(len,off,buf,I,WRITE_MODE);
	ret =iod_blob_write(I->oh,I->tid,hints,I->mem_desc,I->io_desc,I->cksum,event);
	IOD_RETURN_ON_ERROR("iod_blob_write",ret); // successful write returns zero
	*bytes = len;

	return ret;
}

int 
iod_read(iod_state_t *s, char *buf,size_t len,off_t off,ssize_t *bytes) {
	iod_hint_list_t *hints = NULL;
	iod_event_t *event = NULL;
	iod_ret_t ret = 0;

	setup_blob_io(len,off,buf,s,READ_MODE);
	ret=iod_blob_read(s->oh,s->tid,hints,s->mem_desc,s->io_desc,s->cksum,event);
	if ( ret == 0 ) { // current semantic is 0 for success
		*bytes = len;
	} else {
		IOD_PRINT_ERR("iod_blob_read",ret);
	}
	
	if (s->params.checksum) {
		plfs_error_t pret = 0;
		pret = plfs_checksum_match(buf,len,(Plfs_checksum)(*s->cksum));
		IOD_RETURN_ON_ERROR("plfs_checksum_match", pret);
	}
	return ret; 
}

int
iod_open(struct Parameters *p, struct State *s, char *filename, int read_write,MPI_Comm mcom) {
	int ret = 0;

	if ( read_write == WRITE_MODE ) {
		return open_wr(&(s->iod_state),filename,mcom);
	} else {
		return open_rd(&(s->iod_state),filename,mcom);
	}

}

#endif
