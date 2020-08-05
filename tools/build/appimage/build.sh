#!/bin/sh

mkdir -p virtualxt-appimage/lib/x86_64-linux-gnu

cp /lib/x86_64-linux-gnu/libSDL2-2.0.so.0 virtualxt-appimage/lib/x86_64-linux-gnu

cp ../../../virtualxt virtualxt-appimage/virtualxt
cp ../../../doc/icon/icon.png virtualxt-appimage/virtualxt-icon.png

/home/phix/Desktop/appimagetool-x86_64.AppImage virtualxt-appimage

rm virtualxt-appimage/virtualxt
rm virtualxt-appimage/virtualxt-icon.png

rm -r virtualxt-appimage/lib