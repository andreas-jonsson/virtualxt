/*
Copyright (c) 2019-2020 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

package main

import (
	"bytes"
	"flag"
	"io"
	"log"
	"os"
	"os/exec"
)

var (
	outputFile = "../../vxtx.bin"
	inputFile  = "../vxtx.asm"
)

func init() {
	flag.StringVar(&outputFile, "out", outputFile, "")
	flag.StringVar(&inputFile, "in", inputFile, "")
}

func main() {
	flag.Parse()
	log.Print("Building: " + inputFile)

	var out bytes.Buffer
	cmd := exec.Command("nasm", "-o", outputFile, "-l", outputFile+".lst", inputFile)
	cmd.Stderr = &out

	if cmd.Run() != nil {
		log.Print(out.String())
		os.Exit(-1)
	}

	fp, err := os.OpenFile(outputFile, os.O_RDWR, 0644)
	if err != nil {
		log.Print(err)
		os.Exit(-1)
	}
	defer fp.Close()

	size, err := fp.Seek(0, io.SeekEnd)
	if err != nil {
		log.Print(err)
		os.Exit(-1)
	}

	if _, err := fp.Seek(0, io.SeekStart); err != nil {
		log.Print(err)
		os.Exit(-1)
	}

	var (
		sum byte
		buf [1]byte
	)

	for i := 0; i < int(size-1); i++ {
		if _, err := fp.Read(buf[:]); err != nil {
			log.Print(err)
			os.Exit(-1)
		}
		sum += buf[0]
	}

	buf[0] = byte(256 - int(sum))
	if _, err := fp.WriteAt(buf[:], size-1); err != nil {
		log.Print(err)
		os.Exit(-1)
	}

	log.Printf("Checksum: 0x%X", buf[0])
}
