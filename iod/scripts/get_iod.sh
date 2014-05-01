#! /bin/bash

. /scratch/iod/scripts/funcs.sh

cd $TOP
\rm -rf *
proxied_git clone git://git.whamcloud.com/ff/iod.git -b master
chmod 0777 iod
cd iod/src/dep
git clone https://github.com/zhang-jingwang/plfs-core.git plfs -b iod
chmod 0777 plfs
