#!/bin/sh

APP_DIR="$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage-build"
rm -rf $APP_DIR VirtualXT-x86_64.AppImage

cp -r "$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage" $APP_DIR
mkdir -p $APP_DIR/usr/lib/x86_64-linux-gnu $APP_DIR/lib64 $APP_DIR/bios $APP_DIR/boot

# Expect this to be on an Ubuntu system!
DST="$APP_DIR/usr/lib/x86_64-linux-gnu"
SRC="/usr/lib/x86_64-linux-gnu"

cp -a "$GITHUB_WORKSPACE/sdl_bin/lib/libSDL2*.so*" $APP_DIR/usr/lib

#cp -a $SRC/libc.so* $DST
#cp -a $SRC/libm.so* $DST
#cp -a /lib64/ld-linux-x86-64.so* $APP_DIR/lib64/

# Required for network support.
cp -a $SRC/libpcap.so* $DST
cp -a $SRC/libdbus-1.so* $DST
cp -a $SRC/libsystemd.so* $DST
cp -a $SRC/liblzma.so* $DST
cp -a $SRC/libzstd.so* $DST
cp -a $SRC/liblz4.so* $DST
cp -a $SRC/libcap.so* $DST
cp -a $SRC/libgcrypt.so* $DST
cp -a $SRC/libgpg-error.so* $DST

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