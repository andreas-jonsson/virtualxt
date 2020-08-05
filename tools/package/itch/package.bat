@echo off

set PACKAGE_DEST="%TRAVIS_BUILD_DIR%\package\virtualxt"
if exist %PACKAGE_DEST%\ rmdir /q /s %PACKAGE_DEST%
mkdir %PACKAGE_DEST%

copy virtualxt.exe %PACKAGE_DEST%
rem xcopy /S /Y /I doc\manual %PACKAGE_DEST%\manual
copy boot\freedos\freedos.img %PACKAGE_DEST%\freedos.img
copy tools\package\itch\itch.windows.toml %PACKAGE_DEST%\.itch.toml

copy SDL2-2.0.10\x86_64-w64-mingw32\bin\SDL2.dll %PACKAGE_DEST%