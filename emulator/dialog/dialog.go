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

package dialog

import (
	"flag"
	"io"
	"os"
	"os/exec"
	"runtime"
	"sync/atomic"
)

type DiskController interface {
	Eject(dnum byte) (io.ReadWriteSeeker, error)
	Replace(dnum byte, disk io.ReadWriteSeeker) error
}

var FloppyController DiskController

var (
	mainMenuWasOpen,
	requestRestart int32
)

var defaultFloppyImage = "boot/freedos.img"

var DriveImages [0x100]struct {
	Name string
	Fp   *os.File
}

func init() {
	if p, ok := os.LookupEnv("VXT_DEFAULT_FLOPPY_IMAGE"); ok {
		defaultFloppyImage = p
	}

	flag.StringVar(&DriveImages[0x0].Name, "a", defaultFloppyImage, "Mount image as floppy A")
	flag.StringVar(&DriveImages[0x1].Name, "b", "", "Mount image as floppy B")
	flag.StringVar(&DriveImages[0x80].Name, "c", "", "Mount image as haddrive C")
	flag.StringVar(&DriveImages[0x81].Name, "d", "", "Mount image as haddrive D")
}

func OpenURL(url string) error {
	var (
		cmd  string
		args []string
	)

	switch runtime.GOOS {
	case "windows":
		cmd = "cmd"
		args = []string{"/c", "start"}
	case "darwin":
		cmd = "open"
	default:
		cmd = "xdg-open"
	}

	if err := exec.Command(cmd, append(args, url)...).Start(); err != nil {
		return err
	}
	return nil
}

func MainMenuWasOpen() bool {
	return atomic.LoadInt32(&mainMenuWasOpen) != 0
}

func RestartRequested() bool {
	return atomic.SwapInt32(&requestRestart, 0) != 0
}
