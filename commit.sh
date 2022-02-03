#!/bin/bash
VER="$(python3 increment.py $1)"
echo $VER
git commit -a -m "$VER $2"
git push