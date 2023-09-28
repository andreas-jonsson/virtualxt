#!/bin/bash

HERE="$(cd "$(dirname "$0")" && pwd -P)"
CONFIG=$($HERE/virtualxt --locate)

mkdir -p $CONFIG/boot
cp -n $HERE/../Resources/boot/freedos_hd.img $CONFIG/boot/

export VXT_DEFAULT_HD_IMAGE=$CONFIG/boot/freedos_hd.img

exec "${HERE}/virtualxt" "$@"