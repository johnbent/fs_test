#ifdef HAS_IOD

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "iod.h"

int
init_iod( struct Parameters *p, struct State *s ) {
	fprintf( stderr, "%d: About to init iod with %d ranks\n", s->my_rank, p->num_procs_world );
	MPI_Comm comm = MPI_COMM_WORLD;
	int ret = iod_initialize(comm, NULL, 
			p->num_procs_world, p->num_procs_world );
	if ( ret != 0 ) return ret;

	/* open the container */
	iod_hint_list_t *con_open_hint = NULL, *obj_create_hint = NULL;
	/*
	con_open_hint = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
    con_open_hint->num_hint = 1;
    con_open_hint->hint[0].key = "iod_hint_co_scratch_cksum";
	*/

    obj_create_hint = (iod_hint_list_t *)malloc(sizeof(iod_hint_list_t) + sizeof(iod_hint_t));
    obj_create_hint->num_hint = 1;
    obj_create_hint->hint[0].key = "iod_hint_obj_enable_cksum";

	char *target = expand_path( p->tfname, p->test_time, p->num_nn_dirs, s );
	fprintf( stderr, "About to open container %s with %d ranks\n", target, p->num_procs_world );
    ret = iod_container_open(target, con_open_hint, IOD_CONT_CREATE|IOD_CONT_RW,
                             &(s->iod_items.iod_coh), NULL);
	if ( ret != 0 ) return ret;

	/* skip TID=0 */
	if (!s->my_rank) { 
		fprintf( stderr, "About to start tid=0 on container %s with %d ranks\n", target, p->num_procs_world );
		s->iod_items.iod_tid = 0;
		int mpi_size = p->num_procs_world;
		ret = iod_trans_start(s->iod_items.iod_coh, &(s->iod_items.iod_tid), NULL, 0, IOD_TRANS_W, NULL);
		if(ret != 0) {
			printf("iod_trans_start failed, ret: %d (%s).\n",
				   ret, strerror(-ret));
			assert(0);
		}
		fprintf( stderr, "About to finish tid=0 on container %s with %d ranks\n", target, p->num_procs_world );
		ret = iod_trans_finish(s->iod_items.iod_coh, s->iod_items.iod_tid, NULL, 0, NULL);
		if(ret != 0) {
			printf("iod_tr finish failed, ret: %d (%s).\n",
				   ret, strerror(-ret));
			assert(0);
		}
	}

	return ret;
}

int
iod_open(struct Parameters *p, struct State *s, char *filename, int read_write) {
	return 0;
}

#endif
