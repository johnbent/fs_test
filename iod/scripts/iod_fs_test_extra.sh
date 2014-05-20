#! /bin/bash

# find iodrc
source /scratch/iod/scripts/funcs.sh

awk '/^central_io_buffering/ {print "daos_buffering "$2} \
	 /^iod_threadpool_size/  {print} \
	 /^iod_checksum/ {print} \
	' $iodrc
#egrep 'threadpool|checksum|buffering' /etc/iodrc | grep -v '^#'

# plfs_check_config finds plfsrc for us
plfs_check_config | awk '/^Threadpool/ {print "plfs_threads "$3} \
						 /^Max index/  {print "plfs_cksum_sz " $5}'
