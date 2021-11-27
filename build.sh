#!/bin/bash

DIR=${PWD##*/}

if [[ $1 = "-c" ]] ; then
sudo rm -f -r ./build
sudo rm -f -r ./CMakeFiles
sudo rm -f CMakeCache.txt
fi

cmake --clean-first ../$DIR
cmake --build ../$DIR