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
	"log"
	"os"
	"runtime"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/dialog"
	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/debug"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/disk"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/joystick"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/keyboard"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pic"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pit"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/ram"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/rom"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/smouse"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/speaker"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/video/mda"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/cpu"
)

var (
	biosImage  = "bios/pcxtbios.bin"
	vbiosImage = "bios/vxtcga.bin"
)

var (
	limitMIPS   float64
	v20cpu, man bool
)

func init() {
	if p, ok := os.LookupEnv("VXT_DEFAULT_BIOS_PATH"); ok {
		biosImage = p
	}

	if p, ok := os.LookupEnv("VXT_DEFAULT_VIDEO_BIOS_PATH"); ok {
		vbiosImage = p
	}

	flag.BoolVar(&v20cpu, "v20", false, "Emulate NEC V20 CPU")
	flag.BoolVar(&man, "m", false, "Open manual")

	flag.Float64Var(&limitMIPS, "mips", 0, "Limit CPU speed")
	flag.StringVar(&biosImage, "bios", biosImage, "Path to BIOS image")
	flag.StringVar(&vbiosImage, "vbios", vbiosImage, "Path to EGA/VGA BIOS image")

	if !mdaVideo {
		flag.BoolVar(&mdaVideo, "mda", false, "Emulate MDA video in termainal mode")
	}
}

func emuLoop() {
	if man {
		dialog.OpenManual()
		return
	}

	bios, err := os.Open(biosImage)
	if err != nil {
		dialog.ShowErrorMessage(err.Error())
		return
	}
	defer bios.Close()

	videoBios, err := os.Open(vbiosImage)
	if err != nil {
		dialog.ShowErrorMessage(err.Error())
		return
	}
	defer videoBios.Close()

	dc := &disk.Device{BootDrive: 0xFF}
	dialog.FloppyController = dc

	for i, v := range dialog.DriveImages {
		if v.Name != "" {
			name := v.Name
			bootable := false
			if name[0] == '*' {
				bootable = true
				name = name[1:]
			}

			var err error
			if dialog.DriveImages[i].Fp, err = os.OpenFile(name, os.O_RDWR, 0644); err != nil {
				dialog.ShowErrorMessage(err.Error())
			} else if err = dc.Insert(byte(i), dialog.DriveImages[i].Fp); err != nil {
				dialog.ShowErrorMessage(err.Error())
			} else if bootable || dc.BootDrive == 0xFF {
				dc.BootDrive = byte(i)
			}
		}
	}

	if dc.BootDrive == 0xFF {
		dialog.ShowErrorMessage("No boot device selected!")
		return
	}

	if !checkBootsector(dc) {
		dialog.ShowErrorMessage("The selected disk is not bootable!")
		return
	}

	video := defaultVideoDevice()
	if mdaVideo {
		video = &mda.Device{}
	}
	debug.MuteLogging(mdaVideo)

	spkr := &speaker.Device{}

	peripherals := []peripheral.Peripheral{
		&ram.Device{}, // RAM (needs to go first since it maps the full memory range)
		&rom.Device{
			RomName: "BIOS",
			Base:    memory.NewPointer(0xFE00, 0),
			Reader:  bios,
		},
		&rom.Device{
			RomName: "Video BIOS",
			Base:    memory.NewPointer(0xC000, 0),
			Reader:  videoBios,
		},
		&pic.Device{},      // Programmable Interrupt Controller
		&pit.Device{},      // Programmable Interval Timer
		dc,                 // Disk Controller
		video,              // Video Device
		spkr,               // PC Speaker
		&keyboard.Device{}, // Keyboard Controller
		&joystick.Device{}, // Game Port Joystick
		&smouse.Device{ // Microsoft Serial Mouse (COM1)
			BasePort: 0x3F8,
			IRQ:      4,
		},
	}
	if debug.EnableDebug {
		peripherals = append(peripherals, &debug.Device{})
	}

	var doLimit float64 = limitMIPS
	if doLimit == 0 {
		doLimit = 0.33
	}
	limitSpeed := 1000000000 / int64(1000000*doLimit)

	p := cpu.NewCPU(peripherals)
	defer p.Close()

	p.SetV20Support(v20cpu)
	p.Reset()

	for !dialog.ShutdownRequested() {
		var cycles int64
		t := time.Now().UnixNano()

		if dialog.RestartRequested() {
			p.Reset()
		}

	step:
		c, err := p.Step()
		if err != nil {
			log.Print(err)
			return
		}
		if limitMIPS == 0 && spkr.TurboSwitch() {
			continue
		}
		cycles += int64(c)

	wait:
		if n := time.Now().UnixNano() - t; n <= 0 {
			runtime.Gosched()
			goto step
		} else if n < limitSpeed*cycles {
			goto wait
		}
	}
}

func checkBootsector(dc *disk.Device) bool {
	fp := dialog.DriveImages[dc.BootDrive].Fp
	var sector [512]byte
	fp.ReadAt(sector[:], 0)
	fp.Seek(0, os.SEEK_SET)
	return sector[511] == 0xAA && sector[510] == 0x55
}
