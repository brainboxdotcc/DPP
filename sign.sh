#!/bin/sh
rm -rf build/sign
mkdir -p build/sign
cd build/sign
gh release download "$1"
rm -fv *.asc
find . -type f -exec gpg --armor --detach-sign {} \;
gh release upload "$1" *.asc

