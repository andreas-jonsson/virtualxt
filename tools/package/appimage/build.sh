#!/bin/sh

APP_DIR="$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage-build"
rm -rf $APP_DIR VirtualXT-x86_64.AppImage

cp -r "$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage" $APP_DIR
mkdir -p $APP_DIR/usr/lib/x86_64-linux-gnu $APP_DIR/lib64 $APP_DIR/bios $APP_DIR/boot

# Expect this to be on an Ubuntu system!
DST="$APP_DIR/usr/lib/x86_64-linux-gnu"
SRC="/usr/lib/x86_64-linux-gnu"

#cp $SRC/libc.so* $APP_DIR/lib64/
#cp $SRC/libm.so* $APP_DIR/lib64/
cp /lib64/ld-linux-x86-64.so* $APP_DIR/lib64/

# Required for network support.
cp $SRC/libpcap.so* $DST
cp $SRC/libdbus-1.so* $DST
cp $SRC/libsystemd.so* $DST
cp $SRC/liblzma.so* $DST
cp $SRC/libzstd.so* $DST
cp $SRC/liblz4.so* $DST
cp $SRC/libcap.so* $DST
cp $SRC/libgcrypt.so* $DST
cp $SRC/libgpg-error.so* $DST

cp build/bin/virtualxt $APP_DIR/
cp bios/pcxtbios.bin $APP_DIR/bios/
cp bios/pcxtbios_640.bin $APP_DIR/bios/
cp bios/glabios.bin $APP_DIR/bios/
cp bios/vxtx.bin $APP_DIR/bios/
cp boot/freedos_hd.img $APP_DIR/boot/
cp tools/icon/icon.png $APP_DIR/virtualxt-icon.png

curl -L -o apptool.AppImage https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x apptool.AppImage

ARCH=x86_64 ./apptool.AppImage $APP_DIR

rm apptool.AppImage
rm -rf $APP_DIR