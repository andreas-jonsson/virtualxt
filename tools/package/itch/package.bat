@echo off

set PACKAGE_DEST="%GITHUB_WORKSPACE%\package\virtualxt"
if exist %PACKAGE_DEST%\ rmdir /q /s %PACKAGE_DEST%
mkdir %PACKAGE_DEST%\bios %PACKAGE_DEST%\boot

copy build\bin\virtualxt.exe %PACKAGE_DEST%
copy bios\pcxtbios.bin %PACKAGE_DEST%\bios
copy bios\pcxtbios_640.bin %PACKAGE_DEST%\bios
copy bios\glabios.bin %PACKAGE_DEST%\bios
copy bios\vxtx.bin %PACKAGE_DEST%\bios
copy boot\freedos_hd.img %PACKAGE_DEST%\boot
copy tools\package\itch\itch.windows.toml %PACKAGE_DEST%\.itch.toml

copy build\bin\virtualxt.exe %PACKAGE_DEST%\virtualxt-net.exe
copy tools\npcap\npcap-1.72.exe %PACKAGE_DEST%\npcap-installer.exe
copy tools\package\windows\virtualxt-net.bat %PACKAGE_DEST%
copy tools\package\windows\virtualxt-net.exe.manifest %PACKAGE_DEST%
