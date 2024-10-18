@echo off

set PACKAGE_DEST="%GITHUB_WORKSPACE%\package\virtualxt"
if exist %PACKAGE_DEST%\ rmdir /q /s %PACKAGE_DEST%
mkdir %PACKAGE_DEST%\bios %PACKAGE_DEST%\boot

copy "%GITHUB_WORKSPACE%\SDL2-%SDL2_VERSION%\x86_64-w64-mingw32\bin\SDL2.dll" %PACKAGE_DEST%

copy build\sdl2\virtualxt.exe %PACKAGE_DEST%
copy build\ebridge\ebridge.exe %PACKAGE_DEST%
copy bios\pcxtbios.bin %PACKAGE_DEST%\bios
copy bios\GLABIOS.ROM %PACKAGE_DEST%\bios
copy bios\GLaTICK_0.8.4_AT.ROM %PACKAGE_DEST%\bios
copy bios\vxtx.bin %PACKAGE_DEST%\bios
copy bios\vgabios.bin %PACKAGE_DEST%\bios
copy boot\freedos.img %PACKAGE_DEST%\boot
copy boot\freedos_hd.img %PACKAGE_DEST%\boot
copy boot\elks.img %PACKAGE_DEST%\boot
copy tools\package\itch\itch.windows.toml %PACKAGE_DEST%\.itch.toml
copy tools\npcap\npcap-1.72.exe %PACKAGE_DEST%\npcap-installer.exe
