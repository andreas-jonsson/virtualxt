// +build !sdl

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
	"fmt"
	"sync/atomic"
)

func MainMenu() error {
	atomic.StoreInt32(&mainMenuWasOpen, 1)
	return nil
}

func EjectFloppy() error {
	return nil
}

func MountFloppyImage(file string) error {
	return nil
}

func WindowsInstallNpcap() {
}

func ShowErrorMessage(msg string) error {
	fmt.Println(msg)
	return nil
}

func AskToQuit() bool {
	atomic.StoreInt32(&quitFlag, 1)
	return true
}
