#! /bin/bash

. /scratch/iod/scripts/funcs.sh

cd $IOD_INSTALL_TOP
\rm -rf *

# mchecksum
git clone http://git.mcs.anl.gov/radix/mchecksum

# fs_test
git clone https://github.com/johnbent/fs_test.git -b iod

# IOD
proxied_git clone git://git.whamcloud.com/ff/iod.git -b master
chmod 0777 iod

# PLFS
cd iod/src/dep
git clone https://github.com/zhang-jingwang/plfs-core.git plfs -b iod
chmod 0777 plfs
cd $IOD_INSTALL_TOP
ln -s iod/src/dep/plfs .

