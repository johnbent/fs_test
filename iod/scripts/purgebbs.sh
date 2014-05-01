#! /bin/bash

# figure out where we are. source the funcs.sh to get iodrc and helper funcs 
mydir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $mydir/funcs.sh

# notice the script acts differently if the local arg is passed
if [[ $1 == "local" ]]; then
	Purge
else
	mydir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	myname=`basename $0`
	for ion in `echo $EFF_MPI_IONS | sed 's/,/ /g'` 
	do
		echo "Purging $ion"
		ssh $ion $mydir/$myname local
	done
fi
