# VirtualXT

**This emulator is still Work-In-Progress!**

VirtualXT is a IBM PC/XT emulator that runs on modern hardware and operating systems.
It is designed to be simple and lightweight yet still capable enough to run a large
library of old application and games.

The emulator is written i pure Go and should compile with only the standard
Go [toolchain](https://golang.org/dl/) installed. Although in that case you are limited to textmode only.
If you want graphics and sound you need to link with the SDL2 library by passing the build tag ```-sdl```.

![dos3 screenshot](doc/screenshots/dos3.png)

![edit screenshot](doc/screenshots/edit.png)