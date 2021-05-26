#!/bin/sh

mkdir -p freedos_hd
mount -o loop,offset=32256 ../../boot/freedos_hd.img freedos_hd