#! /bin/bash

source /scratch/iod/scripts/funcs.sh
echo $iodrc

fs_test=/scratch/iod/code/fs_test/fs_test/fs_test.$MY_MPI_HOST.x
if [[ ! -f $fs_test ]]; then
    echo "fs_test executable not found at $fs_test!"
    exit
fi

daos=`awk '/^central_path/ {print $2}' $iodrc`

stripesz=`awk '/^iod_daos_stripe/ {print $2}' $iodrc`
echo $stripesz
halfstripe=`expr $stripesz / 2`
threestripes=`expr $stripesz \* 3`
echo $halfstripe $threestripes

$fs_test -io iod -target john.%s -op write -size 1M -nobj $halfstripe -type 2 -flatten 1
echo "###############################"
echo "Check whether one shard is larger than the rest"
echo "###"
daos_query.sh `ls -ltr $daos/john* | tail -1 | awk '{print $NF}'` | grep '^Shard'
echo "###############################"
echo "Hit enter to continue to next test"
read prompt

$fs_test -io iod -target john.%s -op write -size 1M -nobj $threestripes -type 2 -flatten 1
echo "###############################"
echo "Check shard sizes.  This should have created three stripes" 
echo "###"
daos_query.sh `ls -ltr $daos/john* | tail -1 | awk '{print $NF}'` | grep '^Shard'

