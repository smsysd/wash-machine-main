#!/bin/bash

DIR=${PWD##*/}

if [[ $1 = "-c" ]] ; then
bash clean.sh
fi
echo 1111111111111111
cmake --build ../$DIR