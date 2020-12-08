/*
Copyright (c) 2019-2020 Andreas T Jonsson

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

package ram

import (
	"crypto/rand"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

const Size = 0x100000 // 1MB

type Device struct {
	Clear bool
	mem   [Size]byte
}

func (m *Device) Install(p processor.Processor) error {
	if !m.Clear {
		rand.Read(m.mem[:]) // Scramble memory.
	}
	return p.InstallMemoryDevice(m, 0x0, Size-1)
}

func (m *Device) Name() string {
	return "RAM"
}

func (m *Device) Reset() {
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	return m.mem[addr]
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	m.mem[addr] = data
}
