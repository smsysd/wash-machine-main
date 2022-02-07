#!/bin/bash

DIR=${PWD##*/}

# cmake --build ../$DIR --target clean
cmake --clean-first ../$DIR

rm -rf ./CMakeFiles
rm -f ./CMakeCache.txt
find ./ -type d -name "CMakeFiles" | xargs rm -rfd
find ./ -type f -name "*.cmake" -delete
find ./ -type f -name "Makefile" -delete
find ./ -type f -name "*.a" -delete
find ./ -type f -name "*.o" -delete