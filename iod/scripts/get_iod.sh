#! /bin/bash

. /scratch/iod/scripts/funcs.sh

cd $TOP
\rm -rf iod mchecksum

# mchecksum
git clone http://git.mcs.anl.gov/radix/mchecksum
chmod 0777 mchecksum

# fs_test
#git clone https://github.com/johnbent/fs_test.git -b iod

# IOD
#proxied_git clone git://git.whamcloud.com/ff/iod.git -b master
git clone git://git.whamcloud.com/ff/iod.git -b master
chmod 0777 iod
cd iod
# this patch is now applied upstream
#patch -p1 < /scratch/iod/scripts/build_with_static_daos.patch

# PLFS
cd src/dep
git clone https://github.com/zhang-jingwang/plfs-core.git plfs -b iod
chmod 0777 plfs

