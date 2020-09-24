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
	"runtime/pprof"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/dialog"
	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/debug"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/disk"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/dma"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/joystick"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/keyboard"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/network"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pic"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pit"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/ram"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/rom"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/smouse"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/speaker"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/video/cgatext"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/cpu"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
	"github.com/andreas-jonsson/virtualxt/version"
)

var (
	biosImage  = "bios/vxtbios.bin"
	vbiosImage = ""
)

var (
	genFd, genHd,
	validatorOutput,
	cpuProfile string
	genHdSize = 10
)

var (
	limitMIPS float64
	v20cpu, noAudio,
	man, ver bool
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
	flag.BoolVar(&ver, "v", false, "Print version information")
	flag.BoolVar(&noAudio, "no-audio", false, "Disable audio")

	flag.Float64Var(&limitMIPS, "mips", 3, "Limit CPU speed (0 for no limit)")
	flag.StringVar(&biosImage, "bios", biosImage, "Path to BIOS image")
	flag.StringVar(&vbiosImage, "vbios", vbiosImage, "Path to EGA/VGA BIOS image")

	flag.StringVar(&genFd, "gen-fd", "", "Create a blank 1.44MB floppy image")
	flag.StringVar(&genHd, "gen-hd", "", "Create a blank 10MB hadrddrive image")
	flag.IntVar(&genHdSize, "gen-hd-size", genHdSize, "Set size of the generated harddrive image in megabytes")

	flag.StringVar(&validatorOutput, "validator", validatorOutput, "Set CPU validator output")
	flag.StringVar(&cpuProfile, "cpu-profile", cpuProfile, "Set CPU profile output")

	if !cgaText {
		flag.BoolVar(&cgaText, "text", false, "CGA textmode runing in termainal")
	}
}

func emuLoop() {
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

	printLogo()

	bios, err := os.Open(biosImage)
	if err != nil {
		dialog.ShowErrorMessage(err.Error())
		return
	}
	defer bios.Close()

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
	if cgaText {
		video = &cgatext.Device{}
	}
	debug.MuteLogging(cgaText)

	var spkr speaker.AudioDevice = &speaker.NullDevice{}
	if !noAudio {
		spkr = &speaker.Device{}
	}

	peripherals := []peripheral.Peripheral{
		&ram.Device{}, // RAM (needs to go first since it maps the full memory range)
		&rom.Device{
			RomName: "BIOS",
			Base:    memory.NewPointer(0xFE00, 0),
			Reader:  bios,
		},
		&pic.Device{},      // Programmable Interrupt Controller
		&pit.Device{},      // Programmable Interval Timer
		&dma.Device{},      // DMA Controller
		dc,                 // Disk Controller
		video,              // Video Device
		spkr,               // PC Speaker
		&keyboard.Device{}, // Keyboard Controller
		&joystick.Device{}, // Game Port Joysticks
		&network.Device{},  // Network Adapter
		&smouse.Device{ // Microsoft Serial Mouse (COM1)
			BasePort: 0x3F8,
			IRQ:      4,
		},
	}
	if vbiosImage != "" {
		videoBios, err := os.Open(vbiosImage)
		if err != nil {
			dialog.ShowErrorMessage(err.Error())
			return
		}
		defer videoBios.Close()

		peripherals = append(peripherals, &rom.Device{
			RomName: "Video BIOS",
			Base:    memory.NewPointer(0xC000, 0),
			Reader:  videoBios,
		})
	}
	if debug.EnableDebug { // Add this last so it can find other devices.
		peripherals = append(peripherals, &debug.Device{})
	}

	var doLimit float64 = limitMIPS
	if doLimit == 0 {
		doLimit = 0.33
	}
	limitSpeed := 1000000000 / int64(1000000*doLimit)

	validator.Initialize(validatorOutput)
	defer validator.Shutdown()

	p, errs := cpu.NewCPU(peripherals)
	defer p.Close()

	for _, err := range errs {
		dialog.ShowErrorMessage(err.Error())
	}

	p.SetV20Support(v20cpu)
	p.Reset()

	if cpuProfile != "" {
		if f, err := os.Create(cpuProfile); err != nil {
			log.Print(err)
		} else {
			pprof.StartCPUProfile(f)
			defer pprof.StopCPUProfile()
		}
	}

	for !dialog.ShutdownRequested() {
		var cycles int64
		t := time.Now().UnixNano()

		if dialog.RestartRequested() {
			p.Reset()
		}

	step:
		c, err := p.Step()
		if err != nil && err != processor.ErrCPUHalt {
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
