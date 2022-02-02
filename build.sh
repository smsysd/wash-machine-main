#!/bin/bash

DIR=${PWD##*/}

if [[ $1 = "-c" ]] ; then
bash clean.sh
fi
cmake --build ../$DIR