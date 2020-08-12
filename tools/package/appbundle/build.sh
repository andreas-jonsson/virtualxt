#!/bin/bash

APP_DIR="$TRAVIS_BUILD_DIR/VirtualXT.app"
rm -rf $APP_DIR

cp -r "$TRAVIS_BUILD_DIR/tools/package/appbundle/virtualxt-appbundle" $APP_DIR
mkdir -p $APP_DIR/Contents/MacOS/bios $APP_DIR/Contents/Resources $APP_DIR/Contents/Frameworks

cp virtualxt $APP_DIR/Contents/MacOS/
cp bios/pcxtbios.bin $APP_DIR/Contents/MacOS/bios/
cp doc/icon/icon.icns $APP_DIR/Contents/Resources/
#cp -r doc/manual $APP_DIR/Contents/Resources

YEAR=$(date +%Y)
rpl e14f19fc-199d-4fb9-b334-aed07b29a113 $YEAR $APP_DIR/Contents/Info.plist
rpl e34f19fc-199d-4fb9-b334-aed07b29a173 $FULL_VERSION $APP_DIR/Contents/Info.plist

# Copy frameworks
#cp -r /Volumes/SDL2/SDL2.framework $APP_DIR/Contents/Frameworks

# Patch lib path
#install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2 $APP_DIR/Contents/MacOS/virtualxt 