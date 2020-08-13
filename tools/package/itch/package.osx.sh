#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST

./tools/package/appbundle/build.sh

cp -r VirtualXT.app $PACKAGE_DEST
cp tools/package/itch/itch.osx.toml $PACKAGE_DEST/.itch.toml