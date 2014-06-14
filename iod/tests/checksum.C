#include <plfs.h>
#include <mchecksum.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include <vector>

#define CHUNK_SIZE 1048576
#define MAX_CHUNK_SIZE (128 * CHUNK_SIZE)

Plfs_checksum cs;
mchecksum_object_t mcs, mcs2;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  std::vector<char *> buffers;

    /* initialize the buffers */
  for (unsigned int size = CHUNK_SIZE; size <= MAX_CHUNK_SIZE; size *= 2) {
    char *buf = new char[size];
    memset(buf, 'A', size);
    buffers.push_back(buf);
  }

    /* do it a few times.  Once for plfs_get_checksum, once for memset */
    for (unsigned int j = 0; j < 4; j++) {
        printf("## Bandwidth using %s\n", 
			j==0?"plfs_get_checksum":
			j==1?"memset":
                        j==2?"crc64":"adler32");
        long totaldata = MAX_CHUNK_SIZE * 8;
	if (j==2) {
	    mchecksum_init("crc64", &mcs);
	} else if (j==3) {
	    mchecksum_init("adler32", &mcs);
        }
        for (unsigned int i = 0, size = CHUNK_SIZE;
            size <= MAX_CHUNK_SIZE;
            size *= 2, i++) 
        {
            double start, duration;
            start = MPI_Wtime();
            for (int data = 0; data < totaldata; data += size) {
		switch(j){
		case 0:
                    plfs_get_checksum(buffers[i], size, &cs);
		    break;
		case 1:
                    memset(buffers[i], 'B', size);
		    break;
		case 2:
		case 3:
		    //assert(mchecksum_reset(mcs) >= 0);
		    assert(mchecksum_update(mcs,buffers[i],size) >= 0);
		    assert(mchecksum_get(mcs, &mcs2, sizeof(mcs2), MCHECKSUM_NOFINALIZE)  >= 0);
		    break;
                default: 
                    assert(0);
                    break;
                }
            }
            duration = MPI_Wtime() - start;
            double band = (totaldata / 1048576.0) / duration;
            printf("IO block size %u: Bandwidth %.3f MB/s\n", size, band);
            if (j==3) {
                delete []buffers[i];
                buffers[i] = NULL;
            }
        }
    }
  MPI_Finalize();
}
