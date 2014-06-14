/*
 *  * A simplest test-case, just for convenience.
 *   */

#include "iod_api.h"
#include "plfs.h"
#include "stdio.h"
#include "errno.h"
#include <string.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

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

void
print_ret(int ret, int rank, char *what) {
    char hostname[1024];
    gethostname(hostname,1024);

    printf("%s:%d %s %s: %s\n",
        hostname, rank, what,
        ((ret==0) ? "success" : "failure"),
        ((ret==0) ? "" : strerror(-ret))
        );
}

int
main(int argc, char **argv) {
    int mpi_size, mpi_rank;
    int multi_thread_provide;
    MPI_Comm comm  = MPI_COMM_WORLD;
    iod_ret_t ret;
    int i;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &multi_thread_provide);
    if (multi_thread_provide != MPI_THREAD_MULTIPLE) {
        fprintf( stderr, "ERROR: Unable to initialize MPI (MPI_Init).\n");
        return 1;
    }
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    ret = iod_initialize(comm, NULL, mpi_size, mpi_size);
    IOD_RETURN_ON_ERROR("iod_initialize",ret);

    assert(argv[1]);
    assert(argv[2] && (strcmp("ro", argv[2])==0 || strcmp("rw", argv[2])==0));

    unsigned int mode;
    char *modestr;
    char msg[1024];
    iod_handle_t coh; 
    mode    = strcmp(argv[2],"rw")==0 ?  IOD_CONT_RW : IOD_CONT_R;
    snprintf(msg,1024,"iod_container_open %s", argv[2]);
    ret = iod_container_open(argv[1], NULL, mode, &coh, NULL);
    print_ret(ret, mpi_rank, msg); 
    if (ret==0) {
        MPI_Barrier(comm);
        ret = iod_container_close(coh,NULL,NULL);
        IOD_RETURN_ON_ERROR("iod_container_close",ret);
    }


    MPI_Barrier(comm);
    if (ret==0) {
            ret = iod_finalize(NULL);
            IOD_RETURN_ON_ERROR("iod_finalize", ret);
    }
    MPI_Finalize();
    return 0;
}
