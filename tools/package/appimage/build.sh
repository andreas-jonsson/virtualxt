#!/bin/sh

APP_DIR="$TRAVIS_BUILD_DIR/package/appimage/virtualxt-appimage"
mkdir -p $APP_DIR/lib/x86_64-linux-gnu

# Should locate this in some way.
cp /usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0 $APP_DIR/lib/x86_64-linux-gnu
cp /usr/lib/x86_64-linux-gnu/libsndio.so.6.1 $APP_DIR/lib/x86_64-linux-gnu

cp ../../../virtualxt $APP_DIR/virtualxt
cp ../../../doc/icon/icon.png $APP_DIR/virtualxt-icon.png

wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage $APP_DIR

rm appimagetool-x86_64.AppImage
rm $APP_DIR/virtualxt
rm $APP_DIR/virtualxt-icon.png
rm -r $APP_DIR/lib