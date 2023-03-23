#!/bin/bash

echo Building $1...
rm -rf build/lib
make CC="zig cc -target $1" AR="zig ar" clean libretro-frontend
mkdir package/virtualxt/$1
cp build/lib/* package/virtualxt/$1