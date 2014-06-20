#! /bin/bash

#echo "Tailing -f.  Kill to see less\n"
#tail -f `ls -ltr /tmp/iod.debug* | grep $LOGNAME | tail -1 | awk '{print $9}'`
less `ls -ltr /tmp/iod.debug* | grep $LOGNAME | tail -1 | awk '{print $9}'`
