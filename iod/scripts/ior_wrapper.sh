#! /bin/bash

out=/tmp/$USER.ior.out.`date +%s`
$* | tee $out 
/scratch/iod/code/fs_test/iod/scripts/ior_to_db.pl < $out >> /scratch/iod/results/ior.$MY_MPI_HOST.db
