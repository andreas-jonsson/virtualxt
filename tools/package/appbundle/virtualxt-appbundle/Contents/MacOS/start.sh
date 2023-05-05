#!/bin/bash

HERE="$(cd "$(dirname "$0")" && pwd -P)"

mkdir -p $HOME/.virtualxt/boot
cp -n $HERE/../Resources/boot/freedos_hd.img $HOME/.virtualxt/boot/

export VXT_DEFAULT_MODULES_PATH=$HERE/../Resources/modules
export VXT_DEFAULT_BIOS_PATH=$HERE/../Resources/bios/pcxtbios.bin
export VXT_DEFAULT_VXTX_BIOS_PATH=$HERE/../Resources/bios/vxtx.bin
export VXT_DEFAULT_HD_IMAGE=$HOME/.virtualxt/boot/freedos_hd.img

exec "${HERE}/virtualxt" "$@"