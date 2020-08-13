#!/bin/bash

PACKAGE_DEST="$TRAVIS_BUILD_DIR/package/virtualxt"
rm -rf $PACKAGE_DEST
mkdir -p $PACKAGE_DEST/bios $PACKAGE_DEST/boot

./tools/package/appbundle/build.sh

cp -r VirtualXT.app $PACKAGE_DEST
cp boot/freedos.img $PACKAGE_DEST/boot/
cp tools/package/itch/itch.osx.toml $PACKAGE_DEST/.itch.toml

cat > $PACKAGE_DEST/Launch.sh << 'EOL'
#!/bin/bash
HERE="$(cd "$(dirname "$0")" && pwd -P)"
exec "${HERE}/VirtualXT.app/Contents/MacOS/start.sh" "$@"
EOL

chmod +x $PACKAGE_DEST/Launch.sh

curl -L -o $PACKAGE_DEST/bios/ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%20EGA%20Wonder%20800%2B%20N1.00.BIN"