#!/bin/bash

DIR=${PWD##*/}

cmake --build ../$DIR --target clean

cmake --clean-first ../$DIR