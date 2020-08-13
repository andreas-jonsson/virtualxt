#!/bin/bash

HERE="$(cd "$(dirname "$0")" && pwd -P)"

export VXT_DEFAULT_BIOS_PATH=${HERE}/bios/pcxtbios.bin
export VXT_DEFAULT_VIDEO_BIOS_PATH=${HERE}/../../../bios/ati_ega_wonder_800_plus.bin
export VXT_DEFAULT_FLOPPY_IMAGE=${HERE}/../../../boot/freedos.img

exec "${HERE}/virtualxt" "$@"