# On 22.05.14, we changed the environment again
# As of M8, we reset up the environment:
# [4/8/14, 2:06:29 PM] Johann Lombardi: 
# lola-[12-19] are IONs
# lola[20-27] are CNs
# two new clusters:
# lola-[12-15], lola[20-23]
# lola-[16-19],lola-[24-27] 
# daos oss nodes
# lola-[2-5] : daos for upper cluster

EFF_ION_LOWER="12 13 14 15"
EFF_CN_LOWER="20 21 22 23"
EFF_ION_UPPER="16 17 18 19"
EFF_CN_UPPER="24 25 26 27"
EFF_DAOS_UPPER="2 3 4 5"

EFF_ION_ALL="$EFF_ION_LOWER $EFF_ION_UPPER"
EFF_CN_ALL="$EFF_CN_LOWER $EFF_CN_UPPER"
EFF_LOWER="$EFF_ION_LOWER $EFF_CN_LOWER"
EFF_UPPER="$EFF_ION_UPPER $EFF_CN_UPPER"

function Hostname_match () {
	local target=$1
	if [ $h = "lola-$target" -o $h = "lola-$target-ib" ]; then
		return 0
	fi
	return 1
}

function Set_cluster () {
	export EFF_IONS=`echo $1 | sed "s/ /,/g"`
	export EFF_CNS=`echo $2 | sed "s/ /,/g"`
	export EFF_CLUSTER=$3
	local bbs=""
	local mpihosts=""
	local hosts=`echo $1 | tac -s' '`
	for i in $hosts	
	do
		mpihosts="lola-$i,$mpihosts"
		# the bb's are numbered 1-8 and the IONS are numbered 12-19
		i=`expr $i - 11`
		bbs="/mnt/lustre$i $bbs"
		# it's different on daos_upper though
	done
	for n in $EFF_DAOS_UPPER
	do
		if Hostname_match $n ; then
			bbs="/mnt/bb/pstore"
		fi
	done
	export EFF_BBS=$bbs
	export EFF_MPI_IONS=$mpihosts

	hosts=`echo $2 | tac -s' '`
	mpihosts=""
	for i in $hosts	
	do
		mpihosts="lola-$i,$mpihosts"
	done
	export EFF_MPI_CNS=$mpihosts
}

# set up the environment variables about which BB's are in the "cluster"
# am I currently in the upper or lower cluster
h=`hostname -s`
export EFF_CLUSTER="none"
for n in  $EFF_ION_LOWER $EFF_CN_LOWER
do
	if Hostname_match $n ; then
		#Set_cluster "$EFF_ION_LOWER" "$EFF_CN_LOWER" "lower"
                Set_cluster "$EFF_ION_ALL" "$EFF_CN_ALL" "all"
	fi
done

for n in  $EFF_ION_UPPER $EFF_CN_UPPER
do
	if Hostname_match $n ; then
		#Set_cluster "$EFF_ION_UPPER" "$EFF_CN_UPPER" "upper"
                Set_cluster "$EFF_ION_ALL" "$EFF_CN_ALL" "all"
	fi
done

for n in $EFF_DAOS_UPPER
do
	if Hostname_match $n ; then
		#Set_cluster "$EFF_DAOS_UPPER" "$EFF_CN_UPPER" "upper"
                Set_cluster "$EFF_DAOS_UPPER" "$EFF_DAOS_UPPER" "all"
	fi
done
