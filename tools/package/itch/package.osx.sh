#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST

./tools/package/appbundle/build.sh

cp -r VirtualXT.app $PACKAGE_DEST
cp tools/package/itch/itch.osx.toml $PACKAGE_DEST/.itch.toml

# Patch resources
mkdir -p $PACKAGE_DEST/VirtualXT.app/Contents/Resources/boot
cp boot/freedos.img $PACKAGE_DEST/VirtualXT.app/Contents/Resources/boot

curl -L -o $PACKAGE_DEST/VirtualXT.app/Contents/Resources/bios/ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%20EGA%20Wonder%20800%2B%20N1.00.BIN"