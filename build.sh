#!/bin/bash

DIR=${PWD##*/}

if [[ $1 = "-c" ]] ; then
bash clean.sh
cmake ../$DIR -DCMAKE_BUILD_TYPE=Release
fi
cmake --build ../$DIR
cd render_ledmatrx
cargo build
cd ..
mv ./render_ledmatrx/target/debug/render_ledmatrx ./mbuild/