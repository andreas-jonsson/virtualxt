#!/bin/sh

APP_DIR="$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage-build"
rm -rf $APP_DIR VirtualXT-x86_64.AppImage

cp -r "$GITHUB_WORKSPACE/tools/package/appimage/virtualxt-appimage" $APP_DIR
mkdir -p $APP_DIR/usr/lib/x86_64-linux-gnu $APP_DIR/lib64 $APP_DIR/bios $APP_DIR/boot

# Expect this to be on an Ubuntu system!
DST="$APP_DIR/usr/lib/x86_64-linux-gnu"
SRC="/usr/lib/x86_64-linux-gnu"

# SDL2 dependencies etc.
cp $SRC/libSDL2*.so* $DST
cp $SRC/libc.so* $DST
cp $SRC/libm.so* $DST
cp $SRC/libasound.so* $DST
cp $SRC/libpulse.so* $DST
cp $SRC/libX11.so* $DST
cp $SRC/libXext.so* $DST
cp $SRC/libXcursor.so* $DST
cp $SRC/libXinerama.so* $DST
cp $SRC/libXi.so* $DST
cp $SRC/libXfixes.so* $DST
cp $SRC/libXrandr.so* $DST
cp $SRC/libXss.so* $DST
cp $SRC/libXxf86vm.so* $DST
cp $SRC/libdrm.so* $DST
cp $SRC/libgbm.so* $DST
cp $SRC/libwayland-egl.so* $DST
cp $SRC/libwayland-client.so* $DST
cp $SRC/libwayland-cursor.so* $DST
cp $SRC/libxkbcommon.so* $DST
cp $SRC/libdecor-0.so* $DST
cp $SRC/libpulsecommon-15.99.so* $DST
cp $SRC/libdbus-1.so* $DST
cp $SRC/libxcb.so* $DST
cp $SRC/libXrender.so* $DST
cp $SRC/libwayland-server.so* $DST
cp $SRC/libexpat.so* $DST
cp $SRC/libstdc++.so* $DST
cp $SRC/libffi.so* $DST
cp $SRC/libsndfile.so* $DST
cp $SRC/libX11-xcb.so* $DST
cp $SRC/libsystemd.so* $DST
cp $SRC/libasyncns.so* $DST
cp $SRC/libapparmor.so* $DST
cp $SRC/libXau.so* $DST
cp $SRC/libXdmcp.so* $DST
cp $SRC/libgcc_s.so* $DST
cp $SRC/libFLAC.so* $DST
cp $SRC/libvorbis.so* $DST
cp $SRC/libvorbisenc.so* $DST
cp $SRC/libopus.so* $DST
cp $SRC/libogg.so* $DST
cp $SRC/liblzma.so* $DST
cp $SRC/libzstd.so* $DST
cp $SRC/liblz4.so* $DST
cp $SRC/libcap.so* $DST
cp $SRC/libgcrypt.so* $DST
cp $SRC/libbsd.so* $DST
cp $SRC/libgpg-error.so* $DST
cp $SRC/libmd.so* $DST

cp /lib64/ld-linux-x86-64.so* $APP_DIR/lib64/

# Required for network support.
cp $SRC/libpcap.so* $DST

cp build/bin/virtualxt $APP_DIR/
cp bios/pcxtbios.bin $APP_DIR/bios/
cp bios/vxtx.bin $APP_DIR/bios/
cp boot/freedos_hd.img $APP_DIR/boot/
cp tools/icon/icon.png $APP_DIR/virtualxt-icon.png

curl -L -o apptool.AppImage https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x apptool.AppImage

ARCH=x86_64 ./apptool.AppImage $APP_DIR

rm apptool.AppImage
rm -rf $APP_DIR