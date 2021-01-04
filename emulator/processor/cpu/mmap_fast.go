// +build fastmem

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

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
)

type memMapLookup struct {
	ram, cga, bios, biosext memory.Memory
}

func (*memMapLookup) initializeMemMap([]peripheral.Peripheral) error {
	return nil
}

func (m *memMapLookup) GetMappedMemoryDevice(addr memory.Pointer) memory.Memory {
	switch {
	case addr >= 0xB8000 && addr <= 0xC0000:
		return m.cga
	case addr >= 0xFE000 && addr <= 0xFFFFF:
		return m.bios
	case addr >= 0xE0000 && addr <= 0xE07FF:
		return m.biosext
	default:
		return m.ram
	}
}

func rangeToError(fromA, fromB, toA, toB memory.Pointer) error {
	if fromA != fromB || toA != toB {
		return errors.New("invalid mapping")
	}
	return nil
}

func (m *memMapLookup) InstallMemoryDevice(device memory.Memory, from, to memory.Pointer) error {
	switch device.(peripheral.Peripheral).Name() {
	case "Color Graphics Adapter":
		m.cga = device
		return rangeToError(from, 0xB8000, to, 0xC0000)
	case "BIOS":
		m.bios = device
		return rangeToError(from, 0xFE000, to, 0xFFFFF)
	case "VirtualXT BIOS Extension":
		m.biosext = device
		return rangeToError(from, 0xE0000, to, 0xE07FF)
	case "RAM":
		m.ram = device
		return nil
	default:
		return errors.New("peripheral is not supported in this static mmap")
	}
}
