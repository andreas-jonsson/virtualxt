/*
Copyright (c) 2019-2021 Andreas T Jonsson

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
	"flag"
	"fmt"
	"os"

	"github.com/andreas-jonsson/virtualxt/emulator"
	"github.com/andreas-jonsson/virtualxt/platform"
	"github.com/andreas-jonsson/virtualxt/platform/dialog"
	"github.com/andreas-jonsson/virtualxt/version"
)

var (
	genFd, genHd string
	genHdSize    = 10
)

var (
	noAudio,
	man,
	ver bool
)

func init() {
	flag.BoolVar(&man, "m", false, "Open manual")
	flag.BoolVar(&ver, "v", false, "Print version information")
	flag.BoolVar(&noAudio, "no-audio", false, "Disable audio")

	flag.StringVar(&genFd, "gen-fd", "", "Create a blank 1.44MB floppy image")
	flag.StringVar(&genHd, "gen-hd", "", "Create a blank 10MB hadrddrive image")
	flag.IntVar(&genHdSize, "gen-hd-size", genHdSize, "Set size of the generated harddrive image in megabytes")

	flag.Bool("text", false, "CGA textmode runing in termainal")
}

func main() {
	flag.Parse()

	if man {
		dialog.OpenManual()
		return
	}

	if ver {
		fmt.Printf("%s (%s)\n", version.Current.FullString(), version.Hash)
		return
	}

	if genImage() {
		return
	}

	var configs []platform.Config

	if !noAudio {
		configs = append(configs, platform.ConfigWithAudio)
	}

	printLogo()
	platform.Start(emulator.Start, configs...)
}

func genImage() bool {
	if genHdSize < 10 {
		genHdSize = 10
	} else if genHdSize > 500 {
		genHdSize = 500
	}

	if genFd != "" {
		fd, err := os.Create(genFd)
		if err == nil {
			defer fd.Close()
			var buffer [0x168000]byte
			_, err = fd.Write(buffer[:])
		}
		if err != nil {
			fmt.Print(err)
		}
		return true
	}

	if genHd != "" {
		hd, err := os.Create(genHd)
		if err == nil {
			defer hd.Close()
			var buffer [0x100000]byte
			for i := 0; i < genHdSize; i++ {
				if _, err = hd.Write(buffer[:]); err != nil {
					break
				}
			}
		}
		if err != nil {
			fmt.Print(err)
		}
		return true
	}

	return false
}

func printLogo() {
	fmt.Print(logo)
	fmt.Println("v" + version.Current.String())
	fmt.Println(" ───────═════ " + version.Copyright + " ══════───────\n")
}

var logo = `
██╗   ██╗██╗██████╗ ████████╗██╗   ██╗ █████╗ ██╗     ██╗  ██╗████████╗
██║   ██║██║██╔══██╗╚══██╔══╝██║   ██║██╔══██╗██║     ╚██╗██╔╝╚══██╔══╝
██║   ██║██║██████╔╝   ██║   ██║   ██║███████║██║      ╚███╔╝    ██║   
╚██╗ ██╔╝██║██╔══██╗   ██║   ██║   ██║██╔══██║██║      ██╔██╗    ██║   
 ╚████╔╝ ██║██║  ██║   ██║   ╚██████╔╝██║  ██║███████╗██╔╝ ██╗   ██║   
  ╚═══╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝`
