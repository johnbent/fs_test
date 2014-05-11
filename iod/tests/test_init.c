/*
 * A simplest test-case, just for convenience.
 */

#include "iod_api.h"
#include "plfs.h"
#include "stdio.h"
#include "errno.h"
#include <string.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>


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

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &multi_thread_provide);
    if (multi_thread_provide != MPI_THREAD_MULTIPLE) {
        fprintf( stderr, "ERROR: Unable to initialize MPI (MPI_Init).\n");
	return 1;
    }
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    ret = iod_initialize(comm, NULL, mpi_size, mpi_size);
    print_ret(ret, mpi_rank, "iod_initialize");
    MPI_Barrier(comm);
    if (ret==0) {
	    ret = iod_finalize(NULL);
	    print_ret(ret, mpi_rank, "iod_finalize");
    }
    MPI_Finalize();
    return 0;
}
