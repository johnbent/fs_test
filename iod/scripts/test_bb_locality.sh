#! /bin/bash

plfs_check_config -mkdir > /dev/null
PLFS=`plfs_check_config | awk '/^Mount/ {print $3}'` 

mount | grep -q "$PLFS"
if [ $? -ne 1 ]
then
    :
	#echo "Not unmounting $PLFS" 
#	umount $PLFS
else
    echo "You'll need to mount $PLFS before running"
fi
#echo "Mounting $PLFS" 
#plfs -oallow_other,big_writes ${PLFS}


ts=`date +%s`
hn=`hostname`
file="$PLFS/$USER.$hn.$ts"
echo "hello from $hn at $ts" > $file 
#file $file
#cat $file
bb=`plfs_query $file | grep data | gawk -F/ '{print $3}'`
echo "$hn is using BB $bb" 
#\rm $PLFS/$ts

# heck, leave it mounted
#umount ${PLFS}

