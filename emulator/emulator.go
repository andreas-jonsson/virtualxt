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

package emulator

import (
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"runtime"
	"runtime/pprof"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/cga"
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
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/cpu"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
	"github.com/andreas-jonsson/virtualxt/platform"
	"github.com/andreas-jonsson/virtualxt/platform/dialog"
)

var (
	biosImage  = "bios/vxtbios.bin"
	vxtxImage  = "bios/vxtx.bin"
	vbiosImage = ""
)

var (
	validatorOutput,
	cpuProfile string
)

var (
	limitMIPS float64
	v20cpu    bool
)

func init() {
	if p, ok := os.LookupEnv("VXT_DEFAULT_BIOS_PATH"); ok {
		biosImage = p
	}

	if p, ok := os.LookupEnv("VXT_DEFAULT_VXTX_BIOS_PATH"); ok {
		vxtxImage = p
	}

	if p, ok := os.LookupEnv("VXT_DEFAULT_VIDEO_BIOS_PATH"); ok {
		vbiosImage = p
	}

	flag.BoolVar(&v20cpu, "v20", false, "Emulate NEC V20 CPU")

	flag.Float64Var(&limitMIPS, "mips", 3, "Limit CPU speed (0 for no limit)")
	flag.StringVar(&biosImage, "bios", biosImage, "Path to BIOS image")
	flag.StringVar(&vxtxImage, "vxtx", vxtxImage, "Path to VirtualXT BIOS extension image")
	flag.StringVar(&vbiosImage, "vbios", vbiosImage, "Path to EGA/VGA BIOS image")

	flag.StringVar(&validatorOutput, "validator", validatorOutput, "Set CPU validator output")
	flag.StringVar(&cpuProfile, "cpu-profile", cpuProfile, "Set CPU profile output")
}

func Start(s platform.Platform) {
	bios, err := s.Open(biosImage)
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
			if dialog.DriveImages[i].Fp, err = s.OpenFile(name, os.O_RDWR, 0644); err != nil {
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

	if f := flag.Lookup("text"); f != nil && f.Value.(flag.Getter).Get().(bool) {
		debug.MuteLogging(true)
	}

	spkr := &speaker.Device{}
	peripherals := []peripheral.Peripheral{
		&ram.Device{ // RAM (needs to go first since it maps the full memory range)
			Clear: runtime.GOOS == "js", // A bug in the JS backend does not allow us to scramble that memory.
		},
		&rom.Device{
			RomName: "BIOS",
			Base:    memory.NewPointer(0xFE00, 0),
			Reader:  bios,
		},
		&pic.Device{},      // Programmable Interrupt Controller
		&pit.Device{},      // Programmable Interval Timer
		&dma.Device{},      // DMA Controller
		dc,                 // Disk Controller
		&cga.Device{},      // Video Device
		spkr,               // PC Speaker
		&keyboard.Device{}, // Keyboard Controller
		&joystick.Device{}, // Game Port Joysticks
		&network.Device{},  // Network Adapter
		&smouse.Device{ // Microsoft Serial Mouse (COM1)
			BasePort: 0x3F8,
			IRQ:      4,
		},
	}
	if vxtxImage != "" {
		vxtxBios, err := s.Open(vxtxImage)
		if err != nil {
			dialog.ShowErrorMessage(err.Error())
			return
		}
		defer vxtxBios.Close()

		peripherals = append(peripherals, &rom.Device{
			RomName: "VirtualXT BIOS Extension",
			Base:    memory.NewPointer(0xE000, 0),
			Reader:  vxtxBios,
		})
	}
	if vbiosImage != "" {
		videoBios, err := s.Open(vbiosImage)
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

	if cpuProfile != "" {
		if f, err := s.Create(cpuProfile); err != nil {
			log.Print(err)
		} else {
			limitMIPS = 0
			pprof.StartCPUProfile(f)
			defer pprof.StopCPUProfile()
		}
	}

	var doLimit float64 = limitMIPS
	if doLimit == 0 {
		doLimit = 0.33
	}
	limitSpeed := 1000000000 / int64(1000000*doLimit)

	validator.Initialize(validatorOutput, validator.DefulatQueueSize, validator.DefaultBufferSize)
	defer validator.Shutdown()

	p, errs := cpu.NewCPU(peripherals)
	defer p.Close()

	for _, err := range errs {
		dialog.ShowErrorMessage(err.Error())
	}

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
		if err != nil && err != processor.ErrCPUHalt {
			log.Print(err)
			return
		}
		cycles += int64(c)

		if runtime.GOOS == "js" {
			// This is to prevent the JS backend from deadlocking.
			if cycles > 1000 {
				time.Sleep(time.Nanosecond)
				continue
			}
			goto step
		} else if limitMIPS == 0 && spkr.TurboSwitch() {
			continue
		}

	wait:
		if n := time.Now().UnixNano() - t; n <= 0 {
			runtime.Gosched()
			goto step
		} else if n < limitSpeed*cycles {
			goto wait
		}
	}
}

func StartDebugTest(name string) {
	bin, err := ioutil.ReadFile(name)
	if err != nil {
		log.Fatal(err)
	}

	p, errs := cpu.NewCPU([]peripheral.Peripheral{
		&ram.Device{Clear: true},
		&rom.Device{
			RomName: fmt.Sprintf("TEST: %s.bin", name),
			Base:    memory.NewPointer(0xF000, 0),
			Reader:  bytes.NewReader(bin),
		},
		&pic.Device{},
		&debug.Device{},
	})
	defer p.Close()

	for _, err := range errs {
		log.Fatal(err)
	}

	debug.EnableDebug = true

	// Tests are written for 80186+ machines.
	p.SetV20Support(true)

	p.Reset()
	p.IP = 0xFFF0
	p.CS = 0xF000

	for {
		if _, err := p.Step(); err != nil {
			if err != processor.ErrCPUHalt {
				log.Fatal(err)
			}
			break
		}
		if p.Registers.Debug {
			log.Fatal("CPU hit breakpoint!")
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
