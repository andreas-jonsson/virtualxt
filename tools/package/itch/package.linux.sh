#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST virtualxt-x86_64.AppImage
mkdir -p $PACKAGE_DEST

exec tools/package/appimage/build.sh

cp virtualxt-x86_64.AppImage $PACKAGE_DEST
#cp -r doc/manual $PACKAGE_DEST
cp boot/freedos/freedos.img $PACKAGE_DEST/freedos.img
cp tools/package/itch/itch.linux.toml $PACKAGE_DEST/.itch.toml
