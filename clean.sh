#!/bin/bash

DIR=${PWD##*/}

cmake --build ../$DIR --target clean
rm -r -f ./CMakeFiles
rm -f ./CMakeCache.txt
cmake --clean-first ../$DIR