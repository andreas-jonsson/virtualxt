#!/bin/sh

GOARCH=wasm GOOS=js go build -o virtualxt.wasm ../../virtualxt.go

mkdir -p bios boot
cp ../../bios/*.bin ./bios
cp ../../boot/*.img ./boot

go run server.go