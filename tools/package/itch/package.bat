@echo off

set PACKAGE_DEST="%TRAVIS_BUILD_DIR%\package\virtualxt"
if exist %PACKAGE_DEST%\ rmdir /q /s %PACKAGE_DEST%
mkdir %PACKAGE_DEST%\bios

copy virtualxt.exe %PACKAGE_DEST%
rem xcopy /S /Y /I doc\manual %PACKAGE_DEST%\manual
copy bios\pcxtbios.bin %PACKAGE_DEST%\bios
copy boot\freedos\freedos.img %PACKAGE_DEST%\freedos.img
copy tools\package\itch\itch.windows.toml %PACKAGE_DEST%\.itch.toml

copy SDL2-2.0.10\x86_64-w64-mingw32\bin\SDL2.dll %PACKAGE_DEST%

curl -L -o %PACKAGE_DEST%\bios\ati_ega_wonder_800_plus.bin "https://github.com/BaRRaKudaRain/PCem-ROMs/raw/master/ATI%%20EGA%%20Wonder%%20800%%2B%%20N1.00.BIN"