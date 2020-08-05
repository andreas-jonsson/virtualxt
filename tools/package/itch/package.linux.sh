#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST virtualxt-x86_64.AppImage
mkdir -p $PACKAGE_DEST/bios

exec tools/package/appimage/build.sh

cp virtualxt-x86_64.AppImage $PACKAGE_DEST
#cp -r doc/manual $PACKAGE_DEST
cp bios/pcxtbios.bin $PACKAGE_DEST/bios
cp boot/freedos/freedos.img $PACKAGE_DEST
cp tools/package/itch/itch.linux.toml $PACKAGE_DEST/.itch.toml

curl -L -o $PACKAGE_DEST/bios/ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%20EGA%20Wonder%20800%2B%20N1.00.BIN"
