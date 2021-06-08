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

package cpu

import (
	"errors"
	"log"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
)

const MaxPeripherals = 32

type CPU struct {
	processor.Registers
	instructionState

	isV20, trap bool

	stats        processor.Stats
	peripherals  []peripheral.Peripheral
	pic          processor.InterruptController
	interceptors [0x100]processor.InterruptHandler

	iomap         [0x10000]byte
	ioPeripherals [MaxPeripherals]memory.IO

	mmap           [0x100000]byte
	memPeripherals [MaxPeripherals]memory.Memory
}

func NewCPU(peripherals []peripheral.Peripheral) (*CPU, []error) {
	p := &CPU{peripherals: peripherals}

	dummyIO := &memory.DummyIO{}
	for i := range p.ioPeripherals[:] {
		p.ioPeripherals[i] = dummyIO
	}

	dummyMem := &memory.DummyMemory{}
	for i := range p.memPeripherals[:] {
		p.memPeripherals[i] = dummyMem
	}

	for i := 1; i <= len(peripherals); i++ {
		if dev, ok := peripherals[i-1].(memory.IO); ok {
			p.ioPeripherals[i] = dev
		}
		if dev, ok := peripherals[i-1].(memory.Memory); ok {
			p.memPeripherals[i] = dev
		}
	}
	return p, p.installPeripherals()
}

func (p *CPU) SetV20Support(b bool) {
	p.isV20 = b
}

func (p *CPU) installPeripherals() []error {
	var errs []error

	log.Print("\nPeripherals")
	for _, d := range p.peripherals {
		log.Print(" |- ", d.Name())
	}
	log.Println("")

	for _, d := range p.peripherals {
		log.Print("Installing ", d.Name(), "...")
		if err := d.Install(p); err != nil {
			errs = append(errs, err)
		}
		if pic, ok := d.(processor.InterruptController); ok {
			p.pic = pic
		}
	}

	if p.pic == nil {
		errs = append(errs, errors.New("No interrupt controller detected!"))
	}
	return errs
}

func (p *CPU) Close() {
	for _, d := range p.peripherals {
		if cd, b := d.(peripheral.PeripheralCloser); b {
			if err := cd.Close(); err != nil {
				log.Print("Failed to close peripheral: ", err)
			}
		}
	}
}

func (p *CPU) Break() {
	p.Debug = true
}

func (p *CPU) GetStats() processor.Stats {
	s := p.stats
	p.stats = processor.Stats{}
	return s
}

func (p *CPU) GetInterruptController() processor.InterruptController {
	return p.pic
}

func (p *CPU) Reset() {
	log.Print("CPU reset!")

	p.halted = false
	p.trap = false
	p.instructionState = instructionState{}
	p.Registers.Reset()

	p.SetCS(0xFFFF)
	for _, d := range p.peripherals {
		d.Reset()
	}
}

func (p *CPU) GetMappedMemoryDevice(addr memory.Pointer) memory.Memory {
	return p.memPeripherals[p.mmap[addr]]
}

func (p *CPU) GetMappedIODevice(port uint16) memory.IO {
	return p.ioPeripherals[p.iomap[port]]
}

func (p *CPU) GetRegisters() *processor.Registers {
	return &p.Registers
}

func (p *CPU) InByte(port uint16) byte {
	p.stats.RX++
	return p.GetMappedIODevice(port).In(port)
}

func (p *CPU) OutByte(port uint16, data byte) {
	p.stats.TX++
	p.GetMappedIODevice(port).Out(port, data)
}

func (p *CPU) InWord(port uint16) uint16 {
	return uint16(p.InByte(port)) | (uint16(p.InByte(port+1)) << 8)
}

func (p *CPU) OutWord(port uint16, data uint16) {
	p.OutByte(port, byte(data&0xFF))
	p.OutByte(port+1, byte(data>>8))
}

func (p *CPU) ReadByte(addr memory.Pointer) byte {
	p.stats.RX++
	addr &= 0xFFFFF
	data := p.GetMappedMemoryDevice(addr).ReadByte(addr)
	validator.ReadByte(uint32(addr), data)
	return data
}

func (p *CPU) WriteByte(addr memory.Pointer, data byte) {
	p.stats.TX++
	addr &= 0xFFFFF
	p.GetMappedMemoryDevice(addr).WriteByte(addr, data)
	validator.WriteByte(uint32(addr), data)
}

func (p *CPU) ReadWord(addr memory.Pointer) uint16 {
	return uint16(p.ReadByte(addr)) | (uint16(p.ReadByte(addr+1)) << 8)
}

func (p *CPU) WriteWord(addr memory.Pointer, data uint16) {
	p.WriteByte(addr, byte(data&0xFF))
	p.WriteByte(addr+1, byte(data>>8))
}

func (p *CPU) InstallInterruptHandler(handler processor.InterruptHandler, num ...int) error {
	for _, n := range num {
		if n > 0xFF || n < 0x0 {
			return errors.New("invalid interrupt number")
		}
		p.interceptors[n] = handler
	}
	return nil
}

func (p *CPU) InstallMemoryDevice(device memory.Memory, from, to memory.Pointer) error {
	for i, d := range p.memPeripherals[:] {
		if d == device {
			for from <= to {
				p.mmap[from] = byte(i)
				from++
			}
			return nil
		}
	}
	return errors.New("could not find peripheral")
}

func (p *CPU) InstallMemoryDeviceAt(device memory.Memory, addr ...memory.Pointer) error {
	for _, a := range addr {
		if err := p.InstallMemoryDevice(device, a, a); err != nil {
			return err
		}
	}
	return nil
}

func (p *CPU) InstallIODevice(device memory.IO, from, to uint16) error {
	for i, d := range p.ioPeripherals[:] {
		if d == device {
			for from <= to {
				p.iomap[from] = byte(i)
				from++
			}
			return nil
		}
	}
	return errors.New("could not find peripheral")
}

func (p *CPU) InstallIODeviceAt(device memory.IO, port ...uint16) error {
	for _, a := range port {
		if err := p.InstallIODevice(device, a, a); err != nil {
			return err
		}
	}
	return nil
}
