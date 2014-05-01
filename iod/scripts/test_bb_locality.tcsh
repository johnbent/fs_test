#! /bin/tcsh

set PLFS = /tmp/iod_plfs

echo "PLFS mounted on $PLFS"

umount ${PLFS}
plfs ${PLFS}

set ts = `date +%s`
set hn = `hostname`
echo "hello from $hn at $ts" > $PLFS/$ts
cat $PLFS/$ts
set bb = `plfs_query $PLFS/$ts | grep data | gawk -F/ '{print $3}'`
echo "$hn is using BB $bb"

umount ${PLFS}

