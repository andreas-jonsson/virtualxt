#!/bin/sh

APP_DIR="$TRAVIS_BUILD_DIR/tools/package/appimage/virtualxt-appimage-build"
rm -rf $APP_DIR VirtualXT-x86_64.AppImage

cp -r "$TRAVIS_BUILD_DIR/tools/package/appimage/virtualxt-appimage" $APP_DIR
mkdir -p $APP_DIR/usr/lib/x86_64-linux-gnu $APP_DIR/bios $APP_DIR/boot

# Required for network support.
cp /usr/lib/x86_64-linux-gnu/libpcap.so $APP_DIR/usr/lib/x86_64-linux-gnu/

cp virtualxt $APP_DIR/
cp bios/vxtbios.bin $APP_DIR/bios/
cp bios/pcxtbios.bin $APP_DIR/bios/
cp bios/vxtcga.bin $APP_DIR/bios/
cp boot/freedos_hd.img $APP_DIR/boot/
cp doc/icon/icon.png $APP_DIR/virtualxt-icon.png
cp -r doc/manual $APP_DIR

curl -L -o apptool.AppImage https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x apptool.AppImage

ARCH=x86_64 ./apptool.AppImage $APP_DIR

rm apptool.AppImage
rm -rf $APP_DIR