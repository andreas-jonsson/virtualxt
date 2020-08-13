#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST

./tools/package/appimage/build.sh

cp VirtualXT-x86_64.AppImage $PACKAGE_DEST/
cp tools/package/itch/itch.linux.toml $PACKAGE_DEST/.itch.toml