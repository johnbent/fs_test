#! /bin/bash

. /scratch/iod/scripts/funcs.sh

set -e
set -x

function myinstall() {
    cd $1
    shift
    \rm -rf build
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX:PATH=$PREFIX $* ..
    make -j8 install

    cd ..
    dname=`basename $PWD`
    gvers=`git rev-parse HEAD`
    date=`date`
    echo "$dname git $gvers built by $USER on $date" |tee $PREFIX/VERSION.$dname
}

# here's the mchecksum cmake line

for PREFIX in $PREFIXES
do
    myinstall $TOP/mchecksum -DBUILD_SHARED_LIBS=ON -DMCHECKSUM_USE_ZLIB=ON 

    myinstall $TOP/iod/src/dep/plfs -DCMAKE_LIBRARY_PATH:PATH=/scratch/iod/lib -DCMAKE_INCLUDE_PATH:PATH=/scratch/iod/include -DCMAKE_BUILD_TYPE=Debug -DADMIN_TOOLS=Yes 
    PLFSPATH=`pwd`

    myinstall $TOP/iod -DENABLE_CORRUPTION_INJECTION=ON -DPLFS_PROJECT_DIR=$PLFSPATH -DDAOS_LIBRARY=/usr/lib64/libdaos.a 
done
