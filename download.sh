#!/bin/bash
HOST=$1
IPADDR=$2
echo $HOST
echo $IPADDR
ssh-copy-id $HOST@$IPADDR

DIR=${PWD}
if [[ $3 = "-c" ]] ; then
bash ./clean.sh
fi

rsync -au ${DIR} $HOST@$IPADDR:

# if [[ $3 = "-c" ]] ; then
# ssh $HOST@$IPADDR "cd wash-machine-main; bash ./build.sh -c"
# else
# ssh $HOST@$IPADDR "cd wash-machine-main; bash ./build.sh"
# fi

