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

package dialog

import (
	"errors"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"sync/atomic"

	"github.com/veandco/go-sdl2/sdl"
)

type buttonList []sdl.MessageBoxButtonData

func (b buttonList) Len() int {
	return len(b)
}

func (b buttonList) Less(i, j int) bool {
	return b[i].ButtonID < b[j].ButtonID
}

func (b buttonList) Swap(i, j int) {
	b[i], b[j] = b[j], b[i]
}

func sortButtons(buttons []sdl.MessageBoxButtonData) []sdl.MessageBoxButtonData {
	if runtime.GOOS != "windows" && runtime.GOOS != "darwin" {
		sort.Sort(sort.Reverse(buttonList(buttons)))
	}
	return buttons
}

func MainMenu() error {
	atomic.StoreInt32(&mainMenuWasOpen, 1)

	buttons := []sdl.MessageBoxButtonData{
		{
			Flags:    sdl.MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
			ButtonID: 0,
			Text:     "Cancel",
		},
		{
			ButtonID: 1,
			Text:     "Reset",
		},
	}

	if DriveImages[0].Fp != nil || DriveImages[1].Fp != nil {
		buttons = append(buttons, sdl.MessageBoxButtonData{
			ButtonID: 2,
			Text:     "Eject",
		})
	}

	buttons = append(buttons, sdl.MessageBoxButtonData{
		ButtonID: 3,
		Text:     "Configure",
	}, sdl.MessageBoxButtonData{
		ButtonID: 4,
		Text:     "Help",
	})

	mbd := sdl.MessageBoxData{
		Flags:   sdl.MESSAGEBOX_INFORMATION,
		Title:   "Menu Options",
		Message: "To mount a new floppy image, drag-n-drop it in the main window.",
		Buttons: sortButtons(buttons),
	}

	if id, err := sdl.ShowMessageBox(&mbd); err == nil {
		switch id {
		case 4:
			return OpenManual()
		case 3:
			if p, err := os.Executable(); err == nil {
				return OpenURL(filepath.Join(filepath.Dir(p), "config.json"))
			}
			return OpenURL("config.json")
		case 2:
			return EjectFloppy()
		case 1:
			atomic.StoreInt32(&requestRestart, 1)
			return nil
		default:
			return errors.New("operation canceled")
		}
	} else {
		return err
	}
}

func EjectFloppy() error {
	buttons := []sdl.MessageBoxButtonData{
		{
			Flags:    sdl.MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
			ButtonID: 0,
			Text:     "Cancel",
		},
	}

	if v := &DriveImages[1]; v.Fp != nil {
		buttons = append(buttons, sdl.MessageBoxButtonData{
			ButtonID: 1,
			Text:     "Drive B",
		})
	}

	if v := &DriveImages[0]; v.Fp != nil {
		buttons = append(buttons, sdl.MessageBoxButtonData{
			ButtonID: 2,
			Text:     "Drive A",
		})
	}

	if len(buttons) <= 1 {
		ShowErrorMessage("No floppy image mounted!")
		return errors.New("no image mounted")
	}

	mbd := sdl.MessageBoxData{
		Flags:   sdl.MESSAGEBOX_INFORMATION,
		Title:   "Eject Floppy",
		Message: "Select floppy drive to eject.",
		Buttons: sortButtons(buttons),
	}

	if id, err := sdl.ShowMessageBox(&mbd); err == nil {
		if id == 0 {
			return errors.New("operation canceled")
		}

		drive := byte(2 - id)
		if _, err := FloppyController.Eject(drive); err != nil {
			ShowErrorMessage(err.Error())
			return err
		}

		DriveImages[drive].Fp.Close()
		DriveImages[drive].Fp = nil
		DriveImages[drive].Name = ""
		return nil
	} else {
		return err
	}
}

func MountFloppyImage(file string) error {
	mbd := sdl.MessageBoxData{
		Flags:   sdl.MESSAGEBOX_INFORMATION,
		Title:   "Mount Floppy Image",
		Message: file,
		Buttons: sortButtons([]sdl.MessageBoxButtonData{
			{
				Flags:    sdl.MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
				ButtonID: 0,
				Text:     "Cancel",
			},
			{
				ButtonID: 1,
				Text: func() string {
					if DriveImages[1].Name == "" {
						return "Drive B"
					}
					return "Drive B*"
				}(),
			},
			{
				Flags:    sdl.MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
				ButtonID: 2,
				Text: func() string {
					if DriveImages[0].Name == "" {
						return "Drive A"
					}
					return "Drive A*"
				}(),
			},
		}),
	}

	if id, err := sdl.ShowMessageBox(&mbd); err == nil {
		if id == 0 {
			return errors.New("operation canceled")
		}

		driveId := byte(2 - id)
		oldFp := DriveImages[driveId].Fp

		if DriveImages[driveId].Fp, err = os.OpenFile(file, os.O_RDWR, 0644); err != nil {
			DriveImages[driveId].Fp = oldFp
			sdl.ShowSimpleMessageBox(sdl.MESSAGEBOX_ERROR, "Error", err.Error(), nil)
			return err
		} else if err = FloppyController.Replace(driveId, DriveImages[driveId].Fp); err != nil {
			sdl.ShowSimpleMessageBox(sdl.MESSAGEBOX_ERROR, "Error", err.Error(), nil)
			return err
		} else if oldFp != nil {
			oldFp.Close()
		}
		return nil
	} else {
		return err
	}
}

func ShowErrorMessage(msg string) error {
	return sdl.ShowSimpleMessageBox(sdl.MESSAGEBOX_ERROR, "Error", msg, nil)
}

func AskToQuit() bool {
	mbd := sdl.MessageBoxData{
		Flags:   sdl.MESSAGEBOX_INFORMATION,
		Title:   "Shutdown",
		Message: "Do you want to shutdown the emulator?",
		Buttons: sortButtons([]sdl.MessageBoxButtonData{
			{
				Flags:    sdl.MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
				ButtonID: 0,
				Text:     "No",
			},
			{
				Flags:    sdl.MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
				ButtonID: 1,
				Text:     "Yes",
			},
		}),
	}

	id, err := sdl.ShowMessageBox(&mbd)
	quit := err != nil || id == 1
	if quit {
		atomic.StoreInt32(&quitFlag, 1)
	}
	return quit
}
