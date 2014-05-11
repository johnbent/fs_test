#include <mpi.h>
#include <stdio.h>
#include <assert.h>

int
main(int argc, char **argv) {
    int mpi_size, mpi_rank, multi_thread_provide,ret;
    char hostname[1024];
    gethostname(hostname,1024);

    ret =MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&multi_thread_provide);
    assert(multi_thread_provide == MPI_THREAD_MULTIPLE && ret==MPI_SUCCESS);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    printf("%d/%d on %s: hello world\n", mpi_rank, mpi_size, hostname);
    assert(MPI_Barrier(MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Finalize() == MPI_SUCCESS);
    return 0;
}
