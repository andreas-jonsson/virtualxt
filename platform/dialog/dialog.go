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

package dialog

import (
	"flag"
	"io"
	"os"
	"os/exec"
	"runtime"
	"sync/atomic"
)

type File interface {
	io.ReadWriteSeeker
	io.ReaderAt
	io.Closer
}

type DiskController interface {
	Eject(dnum byte) (io.ReadWriteSeeker, error)
	Replace(dnum byte, disk io.ReadWriteSeeker) error
}

var (
	OpenFileFunc     func(name string, flag int, perm os.FileMode) (File, error)
	FloppyController DiskController
)

var (
	mainMenuWasOpen,
	requestRestart,
	quitFlag int32
)

var (
	defaultFloppyImage = ""
	defaultHdImage     = "boot/freedos_hd.img"
)

var DriveImages [0x100]struct {
	Name string
	Fp   File
}

func init() {
	if p, ok := os.LookupEnv("VXT_DEFAULT_FLOPPY_IMAGE"); ok {
		defaultFloppyImage = p
	}
	if p, ok := os.LookupEnv("VXT_DEFAULT_HD_IMAGE"); ok {
		defaultHdImage = p
	}

	flag.StringVar(&DriveImages[0x0].Name, "a", defaultFloppyImage, "Mount image as floppy A")
	flag.StringVar(&DriveImages[0x1].Name, "b", "", "Mount image as floppy B")
	flag.StringVar(&DriveImages[0x80].Name, "c", defaultHdImage, "Mount image as haddrive C")
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

func OpenManual() error {
	defaultPath := "manual/index.md.html"
	if p, ok := os.LookupEnv("VXT_DEFAULT_MANUAL_INDEX"); ok {
		defaultPath = p
	}
	return OpenURL(defaultPath)
}

func MainMenuWasOpen() bool {
	return atomic.LoadInt32(&mainMenuWasOpen) != 0
}

func RestartRequested() bool {
	return atomic.SwapInt32(&requestRestart, 0) != 0
}

func ShutdownRequested() bool {
	return atomic.LoadInt32(&quitFlag) != 0
}

func Quit() {
	atomic.StoreInt32(&quitFlag, 1)
}
