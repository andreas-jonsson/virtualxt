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
	"fmt"
	"log"
	"os"
	"runtime"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/debug"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/disk"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/keyboard"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/mda"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pic"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pit"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/ram"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/rom"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/cpu"
)

var (
	biosImage  = "bios/pcxtbios.bin"
	vbiosImage = "bios/ati_ega_wonder_800_plus.bin"
)

var (
	limitMIPS float64
	v20cpu    bool

	driveImage [0x100]struct {
		name string
		fp   *os.File
	}
)

func init() {
	if p, ok := os.LookupEnv("VXT_DEFAULT_BIOS_PATH"); ok {
		biosImage = p
	}

	if p, ok := os.LookupEnv("VXT_DEFAULT_VIDEO_BIOS_PATH"); ok {
		vbiosImage = p
	}

	flag.BoolVar(&v20cpu, "v20", false, "Emulate NEC V20 CPU")

	flag.Float64Var(&limitMIPS, "mips", 0, "Limit CPU speed")
	flag.StringVar(&biosImage, "bios", biosImage, "Path to BIOS image")
	flag.StringVar(&vbiosImage, "vbios", vbiosImage, "Path to EGA/VGA BIOS image")

	flag.StringVar(&driveImage[0x0].name, "a", "", "Mount image as floppy A")
	flag.StringVar(&driveImage[0x1].name, "b", "", "Mount image as floppy B")
	flag.StringVar(&driveImage[0x80].name, "c", "", "Mount image as haddrive C")
	flag.StringVar(&driveImage[0x81].name, "d", "", "Mount image as haddrive D")

	if !mdaVideo {
		flag.BoolVar(&mdaVideo, "mda", false, "Emulate MDA video in termainal mode")
	}
}

func emuLoop() {
	bios, err := os.Open(biosImage)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer bios.Close()

	videoBios, err := os.Open(vbiosImage)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer videoBios.Close()

	dc := &disk.Device{BootDrive: 0xFF}

	for i, v := range driveImage {
		if v.name != "" {
			name := v.name
			bootable := false
			if name[0] == '*' {
				bootable = true
				name = name[1:]
			}

			var err error
			if v.fp, err = os.OpenFile(name, os.O_RDWR, 0644); err != nil {
				fmt.Println(err)
			} else if err = dc.Insert(byte(i), v.fp); err != nil {
				fmt.Println(err)
			} else if bootable || dc.BootDrive == 0xFF {
				dc.BootDrive = byte(i)
			}
		}
	}

	if dc.BootDrive == 0xFF {
		fmt.Println("No boot device!")
		return
	}

	video := defaultVideoDevice()
	if mdaVideo {
		video = &mda.Device{}
	}
	debug.SetTCPLogging(mdaVideo)

	peripherals := []peripheral.Peripheral{
		&ram.Device{}, // RAM (needs to go first since it maps the full memory range)
		&rom.Device{
			RomName: "BIOS",
			//Base:    memory.NewPointer(0xF000, 0),
			Base:   memory.NewPointer(0xFE00, 0),
			Reader: bios,
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
		&keyboard.Device{}, // Keyboard Controller
	}
	if debug.EnableDebug {
		peripherals = append(peripherals, &debug.Device{})
	}

	var limitSpeed int64 = 0
	if limitMIPS != 0 {
		limitSpeed = 1000000000 / int64(1000000*limitMIPS)
	}

	p := cpu.NewCPU(peripherals)
	p.SetV20Support(v20cpu)

	p.Reset()
	//p.IP = 0xFFF0
	//p.CS = 0xF000

	for {
		var cycles int64
		t := time.Now().UnixNano()

	step:
		c, err := p.Step()
		if err != nil {
			log.Print(err)
			return
		}
		if limitSpeed == 0 {
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
