#!/bin/bash

HERE="$(cd "$(dirname "$0")" && pwd -P)"

mkdir -p $HOME/.virtualxt/boot
cp -n $HERE/../Resources/boot/freedos.img $HOME/.virtualxt/boot/

export VXT_DEFAULT_BIOS_PATH=$HERE/../Resources/bios/pcxtbios.bin
export VXT_DEFAULT_VIDEO_BIOS_PATH=$HERE/../Resources/bios/ati_ega_wonder_800_plus.bin
export VXT_DEFAULT_FLOPPY_IMAGE=$HOME/.virtualxt/boot/freedos.img

exec "${HERE}/virtualxt" "$@"