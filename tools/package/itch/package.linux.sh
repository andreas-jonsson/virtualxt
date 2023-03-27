#!/bin/bash

PACKAGE_DEST="$GITHUB_WORKSPACE/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST

./tools/package/appimage/build.sh

cp -a "$GITHUB_WORKSPACE/sdl_bin/lib/libSDL2"*.so* "$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage-build/usr/lib/"

cp VirtualXT-x86_64.AppImage $PACKAGE_DEST/
cp tools/package/itch/itch.linux.toml $PACKAGE_DEST/.itch.toml