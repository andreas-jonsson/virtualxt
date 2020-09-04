#!/bin/bash

HERE="$(cd "$(dirname "$0")" && pwd -P)"

mkdir -p $HOME/.virtualxt/boot
cp -n $HERE/../Resources/boot/freedos.img $HOME/.virtualxt/boot/

export VXT_DEFAULT_BIOS_PATH=$HERE/../Resources/bios/vxtbios.bin
export VXT_DEFAULT_FLOPPY_IMAGE=$HOME/.virtualxt/boot/freedos.img
export VXT_DEFAULT_MANUAL_INDEX=$HERE/../Resources/manual/index.md.html

exec "${HERE}/virtualxt" "$@"