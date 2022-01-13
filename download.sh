#!/bin/bash
HOST=$1
IPADDR=$2
echo $HOST
echo $IPADDR
ssh-copy-id $HOST@$IPADDR

DIR=${PWD}
sudo rm -f -r ./build
sudo rm -f CMakeCache.txt
rsync -au ${DIR} $HOST@$IPADDR:

ssh $HOST@$IPADDR "cd wash-machine-main; bash ./build.sh -c"