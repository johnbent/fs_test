#! /bin/bash

set -x

cd /scratch/iod/fs_test/
RESULTS=iod/results/fs_test.db1
cp -f /scratch/iod/results/fs_test.db $RESULTS 
git add $RESULTS
git commit -m "Updating db results from $MY_MPI_HOST" $RESULTS 
git push origin iod
