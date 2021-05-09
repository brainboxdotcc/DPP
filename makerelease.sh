#!/bin/bash
LASTVER=`git tag | sort -t "." -k1,1n -k2,2n -k3,3n | tail -n1`
MINOR=`echo -n $LASTVER | sed "s/v[0-9]*\.[0-9]*\.//"`
MAJOR=`echo -n $LASTVER | sed "s/\.[0-9]*$//"`
NEWMINOR=$((MINOR + 1))
NEWVER="$MAJOR.$NEWMINOR"
echo "Building and tagging release $NEWVER"
git tag -a "$NEWVER" -m "Release $NEWVER"
git push --tags
cd /tmp
rm -rf /tmp/DPP
git clone --depth=1 --branch "$NEWVER" --recursive git@github.com:brainboxdotcc/DPP.git
cd /tmp/DPP
rm -rf /tmp/DPP/.git*
rm -rf /tmp/DPP/.circleci /tmp/DPP/.gdbargs /tmp/DPP/.vscode /tmp/DPP/makerelease.sh
cd /tmp
tar cvj /tmp/DPP > /tmp/DPP-Release.tar.bz2
mkdir ~/releases
cp /tmp/DPP-Release.tar.bz2 "~/releases/DPP-$NEWVER.tar.bz2"
