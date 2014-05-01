#! /bin/bash

. /scratch/iod/config/setcluster.sh

reserve () {
	if [ $1 == "upper" ]; then 
		nodes=`echo $EFF_UPPER | sed 's/ /,/g'`	
	elif [ $1 == "lower" ]; then
		nodes=`echo $EFF_LOWER | sed 's/ /,/g'`	
	else 
		nodes=$1
	fi
	scontrol create reservation StartTime=now Duration=100000 Nodes=lola-\[$nodes\] Users=$USER
}

if [[ $1 == "get" ]]; then
	if [[ "X$2" == "X" ]]; then
		$0 --help
		exit
	fi
	reserve $2
elif [[ $1 == "list" ]]; then
	showres
elif [[ "$1" == "return" || "$1" == "ret" ]]; then
	for res in `showres | grep $USER | cut -d' ' -f3 | sort | uniq`
	do
		echo $res
		scontrol delete Reservation=$res
	done
elif [[ "$1" == "upper" || "$1" == "lower" ]]; then
	reserve $1
else 
	echo "Usage: $0 get [nodes] | return | list | upper | lower"
	echo "  e.g. $0 get 8,10-12"
fi

#scontrol create reservation StartTime=now Duration=100000 Nodes=lola-[12-15] Users=jlombard

#scontrol delete Reservation=john.bent_65
