#! /bin/bash

source /scratch/iod/tests/funcs.sh

checkroot

function Usage () {
	echo "Usage: $0 <container_path>"
	exit
}

function Parse () {
	local mycommand=$1
	local match=$2
	local field=$3 
	local __resultvar=$4
	myresult=`$mycommand | grep "$match" | cut -d ' ' -f$field` 
	echo "#### $mycommand: $myresult"
	eval $__resultvar=$myresult
}

# test input parameters
test "X$1" == "X" && Usage
cpath=$1
test ! -f $1 && Usage

echo "### ls -l $cpath"
ls -l $cpath
echo "### du -h $cpath"
du -h $cpath

command="daos_ctl run -c $cpath Cor,Cq,Eq,Cc"

# get HCE
Parse "$command" "HCE" 2 hce
echo "HCE is $hce"

# get nshards
Parse "$command" "# shards:" 3 nshards
echo "$nshards shards"

# query each shard
for (( i=0; i < $nshards; i++))
do
	command="daos_ctl run -c $cpath Cor,Sq$hce:$i,Sl$hce:$i,Cc"
	Parse "$command" "Object numbers" 3 nobj
	Parse "$command" "Object numbers" 6 sz
	mbs=$(($sz/1048576))
	objlist=`$command | grep "\[$i:"`
	echo "Shard $i has $nobj objects and uses $sz ($mbs MB)"
	echo "Objects in shard $i: $objlist"
	#echo $command
	#nobj=`$command | grep 
	#$command
done 

exit

$command
# daos_ctl run -c /mnt/daos/containerA Cor,Cq,Sq1:0,Sl1:0,Sl1:1,Sl1:0,Cc
