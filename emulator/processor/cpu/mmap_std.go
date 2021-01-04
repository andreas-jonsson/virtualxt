// +build !fastmem

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
	mmap           [0x100000]byte
	memPeripherals [MaxPeripherals]memory.Memory
}

func (m *memMapLookup) initializeMemMap(peripherals []peripheral.Peripheral) error {
	dummyMem := &memory.DummyMemory{}
	for i := range m.memPeripherals[:] {
		m.memPeripherals[i] = dummyMem
	}

	for i := 1; i <= len(peripherals); i++ {
		if dev, ok := peripherals[i-1].(memory.Memory); ok {
			m.memPeripherals[i] = dev
		}
	}
	return nil
}

func (m *memMapLookup) GetMappedMemoryDevice(addr memory.Pointer) memory.Memory {
	return m.memPeripherals[m.mmap[addr]]
}

func (m *memMapLookup) InstallMemoryDevice(device memory.Memory, from, to memory.Pointer) error {
	for i, d := range m.memPeripherals[:] {
		if d == device {
			for from <= to {
				m.mmap[from] = byte(i)
				from++
			}
			return nil
		}
	}
	return errors.New("could not find peripheral")
}
