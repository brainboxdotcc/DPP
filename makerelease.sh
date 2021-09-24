#!/bin/bash
NEWVER=`cat include/dpp/version.h  | grep DPP_VERSION_TEXT | sed 's/\|/ /' |awk '{print $4}'`
echo "Building and tagging release $NEWVER"
mkdir temp
cd temp
echo "Download assets from CI..."
gh run list -w "D++ CI" | grep $'\t'master$'\t' | grep ^completed | head -n1
gh run download `gh run list -w "D++ CI" | grep $'\t'master$'\t' | grep ^completed | head -n1 | awk '{ printf $(NF-2) }'`
echo "Move assets..."
mkdir assets
mv "./libdpp - Debian Package amd64/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-x64.deb"
mv "./libdpp - Debian Package Linux x86/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-i386.deb"
mv "./libdpp - Debian Package ARM64/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-rpi-arm64.deb"
mv "./libdpp - Debian Package ARMv7 HF/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-rpi-arm7hf.deb"
mv "./libdpp - Debian Package ARMv6/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-rpi-arm6.deb"
mv "./libdpp - Windows x64-Release/libdpp-$NEWVER-win64.zip" "./assets/libdpp-$NEWVER-win64-release-vs2019.zip"
mv "./libdpp - Windows x64-Debug/libdpp-$NEWVER-win64.zip" "./assets/libdpp-$NEWVER-win64-debug-vs2019.zip"
mv "./libdpp - Windows x86-Release/libdpp-$NEWVER-win32.zip" "./assets/libdpp-$NEWVER-win32-release-vs2019.zip"
mv "./libdpp - Windows x86-Debug/libdpp-$NEWVER-win32.zip" "./assets/libdpp-$NEWVER-win32-debug-vs2019.zip"

cd assets

# 64 bit windows
mkdir -p libdpp-$NEWVER-win64/bin
cp ../../win32/bin/*.dll libdpp-$NEWVER-win64/bin
zip -g libdpp-$NEWVER-win64-release-vs2019.zip libdpp-$NEWVER-win64/bin/*
rm -rf libdpp-$NEWVER-win64

mkdir -p libdpp-$NEWVER-win64/bin
cp ../../win32/bin/*.dll libdpp-$NEWVER-win64/bin
zip -g libdpp-$NEWVER-win64-debug-vs2019.zip libdpp-$NEWVER-win64/bin/*
rm -rf libdpp-$NEWVER-win64

# 32 bit windows
mkdir -p libdpp-$NEWVER-win32/bin
cp ../../win32/32/bin/*.dll libdpp-$NEWVER-win32/bin
zip -g libdpp-$NEWVER-win32-release-vs2019.zip libdpp-$NEWVER-win32/bin/*
rm -rf libdpp-$NEWVER-win32

mkdir -p libdpp-$NEWVER-win32/bin
cp ../../win32/32/bin/*.dll libdpp-$NEWVER-win32/bin
zip -g libdpp-$NEWVER-win32-debug-vs2019.zip libdpp-$NEWVER-win32/bin/*
rm -rf libdpp-$NEWVER-win32

cd ..
echo "Create release..."
gh release create "v$NEWVER" --draft --title "v$NEWVER release" --notes "$(git log --format="- %s" $(git show-ref | grep refs/tags | tail -n 1 | cut -d ' ' -f 1)..HEAD)" ./assets/*.zip ./assets/*.deb
echo "Cleaning up..."
cd ..
rm -rf temp

