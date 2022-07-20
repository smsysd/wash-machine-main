#!/bin/bash

if [[ $2 = "-l" ]] ; then
VER="$(python3 increment.py $1 -l)"
echo $VER
echo "edit log and press key.."
code ./changelog.md
read -n1 x
else
VER="$(python3 increment.py $1)"
echo $VER
fi

git commit -a -m "$VER"
git push origin feature/board2