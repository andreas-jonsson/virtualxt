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

package ram

import (
	"math/rand"

	"github.com/andreas-jonsson/i8088-core/emulator/memory"
	"github.com/andreas-jonsson/i8088-core/emulator/processor"
)

const Size = 0x100000 // 1MB

type Device struct {
	mem [Size]byte
}

func (m *Device) Install(p processor.Processor) error {
	rand.Read(m.mem[:]) // Scramble memory.
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
