#! /bin/bash

less `ls -ltr /tmp/iod.debug* | grep $LOGNAME | tail -1 | awk '{print $9}'`
