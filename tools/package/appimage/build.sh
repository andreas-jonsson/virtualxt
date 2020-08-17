#!/bin/sh

APP_DIR="$TRAVIS_BUILD_DIR/tools/package/appimage/virtualxt-appimage-build"
rm -rf $APP_DIR VirtualXT-x86_64.AppImage

cp -r "$TRAVIS_BUILD_DIR/tools/package/appimage/virtualxt-appimage" $APP_DIR
mkdir -p $APP_DIR/bios $APP_DIR/boot

cp virtualxt $APP_DIR/
cp bios/pcxtbios.bin $APP_DIR/bios/
cp boot/freedos.img $APP_DIR/boot/
cp doc/icon/icon.png $APP_DIR/virtualxt-icon.png
cp -r doc/manual $APP_DIR

curl -L -o $APP_DIR/bios/ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%20EGA%20Wonder%20800%2B%20N1.00.BIN"

curl -L -o apptool.AppImage https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x apptool.AppImage

ARCH=x86_64 ./apptool.AppImage $APP_DIR

rm apptool.AppImage
rm -rf $APP_DIR