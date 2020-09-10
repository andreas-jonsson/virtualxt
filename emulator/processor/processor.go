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

package processor

import (
	"errors"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
)

var ErrCPUHalt = errors.New("CPU HALT")

type InterruptController interface {
	GetInterrupt() (int, error)
	IRQ(n int)
}

type Processor interface {
	Debug

	InByte(port uint16) byte
	OutByte(port uint16, data byte)
	InWord(port uint16) uint16
	OutWord(port uint16, data uint16)

	ReadByte(addr memory.Pointer) byte
	WriteByte(addr memory.Pointer, data byte)
	ReadWord(addr memory.Pointer) uint16
	WriteWord(addr memory.Pointer, data uint16)

	GetRegisters() *Registers
	GetInterruptController() InterruptController
	GetMappedMemoryDevice(addr memory.Pointer) memory.Memory
	GetMappedIODevice(port uint16) memory.IO

	InstallMemoryDevice(device memory.Memory, from, to memory.Pointer) error
	InstallMemoryDeviceAt(device memory.Memory, addr ...memory.Pointer) error
	InstallIODevice(device memory.IO, from, to uint16) error
	InstallIODeviceAt(device memory.IO, port ...uint16) error
}
