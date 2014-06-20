cname=containerA    

DU="du -hs"

pmount=/mnt/iod_plfs
dmount=/mnt/daos

if [ -e $pmount/$1 ]; then
    cname=$1
else
    echo "Usage: $0 container"
    echo "ENOENT $pmount/$1"
    exit
fi

testdir () {
    if [ -d $1 ]
    then
        $DU $1
    fi
}

testfile () {
    if [ -e $1 ]
    then
        $DU $1
    fi
}

testfile $dmount/$cname
testdir $pmount/$cname
for backend in `plfs_check_config | awk '/Backend:/ { print $2 }'`
do
    testdir $backend/$cname
done

for iroot in `find /mnt/lustr*/iod_root -name $cname 2>&1 | grep $cname`
do
    testdir $iroot
done
