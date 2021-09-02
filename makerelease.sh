#!/bin/bash
NEWVER=`cat include/dpp/version.h  | grep DPP_VERSION_TEXT | sed 's/\|/ /' |awk '{print $4}'`
echo "Building and tagging release $NEWVER"
mkdir temp
cd temp
echo "Download assets from CI..."
gh run list -w "D++ CI" -L 1
gh run download `gh run list -w "D++ CI" -L 1 | grep master | sed 's/\|/ /'|awk '{print $9}'`
echo "Move assets..."
mkdir assets
mv ./libdpp*/* assets/
echo "Create release..."
gh release create "v$NEWVER-test" --draft --title "v$NEWVER release" --notes "Please populate changelog/notes" ./assets/*.zip ./assets/*.deb
echo "Cleaning up..."
cd ..
rm -rf temp

