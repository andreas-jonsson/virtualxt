#!/bin/bash

PACKAGE_DEST="$GITHUB_WORKSPACE/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST

./tools/package/appbundle/build.sh

cp -a "$GITHUB_WORKSPACE/sdl_bin/lib/libSDL2"*.dylib "$GITHUB_WORKSPACE/VirtualXT.app/Contents/MacOS/"

cp -r VirtualXT.app $PACKAGE_DEST
cp tools/package/itch/itch.osx.toml $PACKAGE_DEST/.itch.toml