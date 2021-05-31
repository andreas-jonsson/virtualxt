@echo off

set PACKAGE_DEST="%TRAVIS_BUILD_DIR%\package\virtualxt"
if exist %PACKAGE_DEST%\ rmdir /q /s %PACKAGE_DEST%
mkdir %PACKAGE_DEST%\bios %PACKAGE_DEST%\boot

copy virtualxt.exe %PACKAGE_DEST%
xcopy /S /Y /I doc\manual %PACKAGE_DEST%\manual
copy bios\pcxtbios.bin %PACKAGE_DEST%\bios
copy bios\vxtx.bin %PACKAGE_DEST%\bios
copy boot\freedos_hd.img %PACKAGE_DEST%\boot
copy tools\package\itch\itch.windows.toml %PACKAGE_DEST%\.itch.toml

copy virtualxt.exe %PACKAGE_DEST%\virtualxt-net.exe
copy tools\package\windows\virtualxt-net.bat %PACKAGE_DEST%
copy tools\package\windows\virtualxt-net.exe.manifest %PACKAGE_DEST%
curl -L -o %PACKAGE_DEST%\npcap-installer.exe "https://nmap.org/npcap/dist/npcap-0.9997.exe"
