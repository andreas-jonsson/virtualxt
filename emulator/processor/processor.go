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
