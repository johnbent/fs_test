#! /bin/bash

set -x
set -e

cd /scratch/iod/fs_test/
git pull origin iod

FS_DEST=iod/results/$MY_MPI_HOST.fs_test.db
FS_SRC=/scratch/iod/output/$MY_MPI_HOST.fs_test.db
IOR_SRC=/scratch/iod/results/ior.$MY_MPI_HOST.db
IOR_DST=iod/results/ior.$MY_MPI_HOST.db
wc $FS_SRC $FS_DEST
cp -f $FS_SRC $FS_DEST 
cp -f $IOR_SRC $IOR_DST
wc $FS_SRC $FS_DEST $IOR_SRC $IOR_DST
git add $FS_DEST 
git add $IOR_DST
git commit -m "Updating db results from $MY_MPI_HOST" $FS_DEST $IOR_DST
git push origin iod
