// +build sdl

/*
Copyright (C) 2019-2020 Andreas T Jonsson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package emulator

import (
	"flag"
	"os"

	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/cga"
	"github.com/veandco/go-sdl2/sdl"
)

func Start() {
	flag.Parse()
	sdl.Main(func() {
		if err := sdl.Init(0); err != nil {
			panic(err)
		}
		emuLoop()
		sdl.Quit()
	})
	os.Exit(0) // Callig Exit is required!
}

var mdaVideo = false

func defaultVideoDevice() peripheral.Peripheral {
	return &cga.Device{}
}
