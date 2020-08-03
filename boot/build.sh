#!/bin/sh

nasm -f bin ./boot-os/boot-os.asm -l ./boot-os/boot-os.lst -o boot-os.img
nasm -f bin ./boot-basic/boot-basic.asm -l ./boot-basic/boot-basic.lst -o boot-basic.img
nasm -f bin ./boot-rogue/boot-rogue.asm -l ./boot-rogue/boot-rogue.lst -o boot-rogue.img
cp ./freedos/freedos.img freedos.img