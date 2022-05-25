# virtualxt

[![Build](https://github.com/andreas-jonsson/virtualxt/actions/workflows/build.yml/badge.svg)](https://github.com/andreas-jonsson/virtualxt/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/andreas-jonsson/virtualxt/branch/main/graph/badge.svg?token=miEtCEo0s4)](https://codecov.io/gh/andreas-jonsson/virtualxt)
[![Forum](https://img.shields.io/badge/forum-VOGONS-blueviolet)](https://www.vogons.org/viewforum.php?f=9)
[![Support](https://github.com/BoostIO/issuehunt-materials/raw/master/v1/issuehunt-shield-v1.svg)](https://issuehunt.io/r/andreas-jonsson/virtualxt)

VirtualXT is a IBM PC/XT (8088/V20) emulator that runs on modern hardware and operating systems.
It is designed to be simple and lightweight yet still capable enough to run a large
library of old application and games.

## Features

* Intel 8088 or NEC V20 CPU
* 1MB RAM
* CGA/HGC compatible graphics
* Turbo XT BIOS 3.1 + VXTX
* Keyboard controller with 83-key XT-style keyboard
* Serial port with Microsoft 2-button mouse
* Floppy and hard disk controller
* Ethernet adapter
* PC speaker

## Build

The emulator is written in C11 and should be compiled with the [Zig](https://ziglang.org/) toolchain.
You also need to have [SDL2](https://www.libsdl.org/) installed on your system or pass `-Dsdl_path=<path to lib>` to the compiler.

```
git clone https://github.com/andreas-jonsson/virtualxt.git
cd virtualxt
zig build
```

You can download pre-built binaries from [itch.io](https://phix.itch.io/virtualxt/purchase). OSX and Linux users can also download VirtualXT using [Homebrew](https://brew.sh).

```
brew tap andreas-jonsson/virtualxt
brew install virtualxt
```

If you want to embed the emulator or create a custom frontend you can find libvxt API documentation [here](https://andreas-jonsson.github.io/virtualxt).

## Screenshots

![bios screenshot](screenshots/bios.PNG)

![freedos screenshot](screenshots/freedos.PNG)

![edit screenshot](screenshots/edit.PNG)

![win30setup screenshot](screenshots/win30setup.PNG)

![monkey screenshot](screenshots/monkey.PNG)
