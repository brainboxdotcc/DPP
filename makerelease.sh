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
mv "./libdpp - Debian Package/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-x64.deb"
mv "./libdpp - Debian Package AArch64/libdpp-$NEWVER-Linux.deb" "./assets/libdpp-$NEWVER-linux-rpi-aarch64.deb"
mv "./libdpp - Windows x64/libdpp-$NEWVER-win64.zip" "./assets/libdpp-$NEWVER-win64-release-vs2019.zip"
cd assets
mkdir -p libdpp-$NEWVER-win64/bin
cp ../../win32/bin/*.dll libdpp-$NEWVER-win64/bin
zip -g *.zip libdpp-$NEWVER-win64/bin/*
cd ..
echo "Create release..."
gh release create "v$NEWVER" --draft --title "v$NEWVER release" --notes "$(git log --format="- %s" $(git show-ref | grep refs/tags | tail -n 1 | cut -d ' ' -f 1)..HEAD)" ./assets/*.zip ./assets/*.deb
echo "Cleaning up..."
cd ..
rm -rf temp

