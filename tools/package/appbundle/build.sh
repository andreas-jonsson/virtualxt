#!/bin/bash

APP_DIR="$TRAVIS_BUILD_DIR/VirtualXT.app"
rm -rf $APP_DIR

cp -r "$TRAVIS_BUILD_DIR/tools/package/appbundle/virtualxt-appbundle" $APP_DIR
mkdir -p $APP_DIR/Contents/MacOS $APP_DIR/Contents/Resources/bios $APP_DIR/Contents/Resources/boot

cp virtualxt $APP_DIR/Contents/MacOS/
cp bios/vxtbios.bin $APP_DIR/Contents/Resources/bios/
cp bios/pcxtbios.bin $APP_DIR/Contents/Resources/bios/
cp bios/vxtcga.bin $APP_DIR/Contents/Resources/bios/
cp boot/freedos.img $APP_DIR/Contents/Resources/boot/
cp doc/icon/icon.icns $APP_DIR/Contents/Resources/
cp -r doc/manual $APP_DIR/Contents/Resources

YEAR=$(date +%Y)
rpl e14f19fc-199d-4fb9-b334-aed07b29a113 $YEAR $APP_DIR/Contents/Info.plist
rpl e34f19fc-199d-4fb9-b334-aed07b29a173 $FULL_VERSION $APP_DIR/Contents/Info.plist
