// +build sdl

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

package dialog

import (
	"errors"
	"log"
	"os"
	"os/exec"
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

	buttons = append(buttons,
		/*
			sdl.MessageBoxButtonData{
				ButtonID: 3,
				Text:     "Configure",
			},
		*/
		sdl.MessageBoxButtonData{
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

func WindowsInstallNpcap() {
	if runtime.GOOS == "windows" {
		mbd := sdl.MessageBoxData{
			Flags:   sdl.MESSAGEBOX_ERROR,
			Title:   "Network Error",
			Message: "Npcap is needed for ethernet emulation. Do you want to install it now?",
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
		if err != nil || id == 0 {
			return
		}

		ep, err := os.Executable()
		if err != nil {
			log.Print(err)
			return
		}

		if err := exec.Command(filepath.Join(filepath.Dir(ep), "npcap-installer.exe")).Start(); err != nil {
			log.Print(err)
			return
		}

		// Perhaps do application restart here?
		os.Exit(0)
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

		if DriveImages[driveId].Fp, err = OpenFileFunc(file, os.O_RDWR, 0644); err != nil {
			DriveImages[driveId].Fp = oldFp
			sdl.ShowSimpleMessageBox(sdl.MESSAGEBOX_ERROR, "Error", err.Error(), nil)
			return err
		} else if err = FloppyController.Replace(driveId, DriveImages[driveId].Fp); err != nil {
			sdl.ShowSimpleMessageBox(sdl.MESSAGEBOX_ERROR, "Error", err.Error(), nil)
			return err
		} else if oldFp != nil {
			oldFp.Close()
		}
		DriveImages[driveId].Name = file
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
		Quit()
	}
	return quit
}
