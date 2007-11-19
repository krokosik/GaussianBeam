#! /bin/bash

RELEASEFOLDER="GaussianBeam-$1"

rm -rf $RELEASEFOLDER
mkdir $RELEASEFOLDER
svn list | xargs cp -R --target-directory=$RELEASEFOLDER
find $RELEASEFOLDER | grep "~" | xargs rm -f
find $RELEASEFOLDER | grep .svn$ | xargs rm -rf
rm $RELEASEFOLDER/release.sh
tar -cj $RELEASEFOLDER > $RELEASEFOLDER.tar.bz2
rm -rf $RELEASEFOLDER
