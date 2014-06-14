#include "iod_api.h"
#include "plfs.h"
#include "stdio.h"
#include "errno.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

static int verbose = 0;
const char *cname = NULL;
static char *myhostname = NULL;
static int rank = 0;
static int nrank = 0;


void
print_ret(int ret, char *what, double elapsed) {
  if (!myhostname) {
    myhostname = (char*)malloc(1024);
    gethostname(myhostname,1024);
  }
  if (verbose || rank == 0) {
      printf("%10s:%3d / %3d %20s %7s took %9.4f s\n", 
        myhostname, rank, nrank, what,
        ((ret==0) ? "success" : "failure"),
        ((ret==0) ? "" : strerror(-ret)),
        elapsed );
  }
  if (ret != 0) exit(-1);
}

void
IOD_Barrier(MPI_Comm comm) {
  double timer = MPI_Wtime();
  MPI_Barrier(comm);
  print_ret(0, "MPI_Barrier", MPI_Wtime() - timer);
}

int
main(int argc, char **argv) {
    int multi_thread_provide;
    MPI_Comm comm = MPI_COMM_WORLD;
    iod_ret_t ret;
    double timer;
    int opt;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &multi_thread_provide);
    if (multi_thread_provide != MPI_THREAD_MULTIPLE) {
        fprintf( stderr, "ERROR: Unable to initialize MPI (MPI_Init).\n");
        return 1;
    }
    MPI_Comm_size(comm, &nrank);
    MPI_Comm_rank(comm, &rank);

    while ((opt = getopt(argc, argv, "vc:")) != -1) {
      switch(opt) {
        case 'v': verbose = 1; break;
        case 'c': cname = strdup(optarg); break;
        default: 
          fprintf(stderr, "Usage: %s [-v] [-c container_name]\n", argv[0]);
          exit(EINVAL);
      }
    }
  
    // IOD initialize
    timer = MPI_Wtime();
    ret = iod_initialize(comm, NULL, nrank, nrank);
    print_ret(ret, "iod_initialize", MPI_Wtime() - timer);
    IOD_Barrier(comm);

    // some basic container ops
    if (cname) {
      unsigned int mode = IOD_CONT_CREATE | IOD_CONT_RW;
      iod_handle_t coh;
      iod_trans_id_t tid = 0;
      timer = MPI_Wtime();
      ret = iod_container_open(cname, NULL, mode, &coh, NULL);
      print_ret(ret, "iod_container_open", MPI_Wtime() - timer);
      IOD_Barrier(comm);

      if (rank==0) {
        timer = MPI_Wtime();
        ret = iod_trans_start(coh, &tid, NULL, 0, IOD_TRANS_W, NULL);
        print_ret(ret, "iod_trans_start", MPI_Wtime() - timer);

        timer = MPI_Wtime();
        ret = iod_trans_finish(coh, tid, NULL, 0, NULL);
        print_ret(ret, "iod_trans_finish", MPI_Wtime() - timer);
      } 
      IOD_Barrier(comm);

      timer = MPI_Wtime();
      ret = iod_container_close(coh, NULL, NULL);
      print_ret(ret, "iod_container_close", MPI_Wtime() - timer);
      IOD_Barrier(comm);
    }

    // IOD finalize
    if (ret==0) {
      timer = MPI_Wtime();
	    ret = iod_finalize(NULL);
	    print_ret(ret, "iod_finalize", MPI_Wtime() - timer);
    }
    IOD_Barrier(comm);
    MPI_Finalize();
    return 0;
}
