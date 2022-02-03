#!/bin/bash
VER="$(python3 increment.py $1)"
echo $VER
echo "edit log and press key.."
read -n1 x
git commit -a -m "$VER $2"
git push