#! /bin/bash

# figure out where we are and source the purgebbs.sh to get iodrc and Clean
mydir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $mydir/funcs.sh

# figure out where the daos mount point is and clean it
daos=`grep central_path $iodrc | grep -v '^#' | awk '{print $2}'`
Clean $daos

# now purge the bbs
$mydir/purgebbs.sh
