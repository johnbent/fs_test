
# a global thing to set the iodrc value
iodrc="/etc/iodrc"
[ -f $HOME/.iodrc ] && iodrc=$HOME/.iodrc 
[[ $IODRC ]] && iodrc=$IODRC

# some variables for installing on buffy
TOP=/scratch/iod/code
PREFIXES="/scratch/iod/ /scratch/iod/install/`date +%Y.%m.%e`"

function Clean () {
	local targdir=$1
	if test -n "$(find $targdir -maxdepth 1 -name $USER'*' -print -quit)"
	then
		local count=`ls $targdir/$USER* | wc -l`
		echo "Cleaning $count entries in $HOSTNAME:$targdir"
		/bin/rm -rf $targdir/$USER*
	else
		echo "No $USER files in $HOSTNAME:$targdir"
	fi
}



function Purge () {
	PurgePLFS
	PurgeIOD
}

function PurgeIOD () {
	# set the iodrc file in reverse priority: /etc/, $HOME, environment
	iodroot=`grep 'iod_root' $iodrc | awk '{print $2}'`
	Clean "$iodroot/*"
}

function PurgePLFS () {
	# all the plfs backends
	for i in `plfs_check_config | grep 'Backend:' | awk '{print $2}'` 
	do
		Clean "$i"
	done

	# and the plfs mount point itself (should be empty now)
	pmount=`plfs_check_config | grep 'Mount Point' | awk '{print $3}'`
	Clean "$pmount"

	# this part is a bit ugly since we hardcode the mlog path 
	if test -n "$(find /tmp -maxdepth 1 -name iod.debug'*' -print -quit)"
	then
		local count=`ls -l /tmp/iod.debug* | grep $USER | wc -l` 
		echo "Cleaning $count entries in $HOSTNAME:/tmp/iod.debug.\*"
		for debug in `ls -l /tmp/iod.debug* | grep $USER | awk '{print $9}'`
		do
			/bin/rm $debug
		done
	else 
		echo "No debug files on $HOSTNAME belonging to $USER"
	fi
}

