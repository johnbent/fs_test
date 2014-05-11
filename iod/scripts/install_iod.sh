#! /bin/bash

. /scratch/iod/scripts/funcs.sh

set -e

for PREFIX in $PREFIXES
do
	# build and install PLFS
	cd $TOP
	cd iod/src/dep
	cd plfs
	PLFSPATH=`pwd`
	\rm -rf build
	mkdir build
	cd build
	cmake -DCMAKE_LIBRARY_PATH:PATH=/scratch/iod/lib -DCMAKE_INCLUDE_PATH:PATH=/scratch/iod/include -DCMAKE_INSTALL_PREFIX:PATH=$PREFIX -DCMAKE_BUILD_TYPE=Debug -DADMIN_TOOLS=Yes ..
	make install

	# build and install IOD
	cd $TOP
	cd iod
	\rm -rf build
	mkdir build
	cd build
	cmake -DCMAKE_INSTALL_PREFIX:PATH=$PREFIX -DENABLE_CORRUPTION_INJECTION=ON -DPLFS_PROJECT_DIR=$PLFSPATH ..
	make install
done
