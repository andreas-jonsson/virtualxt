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

//go:generate bash -c "cd boot && ./build.sh"

package main

import (
	"flag"
	"os"
	"runtime"

	"github.com/andreas-jonsson/i8088-core/emulator"
	"github.com/veandco/go-sdl2/sdl"
)

func init() {
	runtime.LockOSThread() // Needed?
}

func main() {
	flag.Parse()
	sdl.Main(emulator.Start)
	os.Exit(0) // Callig Exit is required!
}
