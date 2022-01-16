#!/bin/bash

DIR=${PWD##*/}

if [[ $1 = "-c" ]] ; then
rm -f -r ./build
rm -f -r ./CMakeFiles
rm -f CMakeCache.txt
fi

cmake --clean-first ../$DIR
cmake --build ../$DIR