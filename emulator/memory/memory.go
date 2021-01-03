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

package memory

import (
	"fmt"
	"log"
)

type Address uint32

func NewAddress(seg, offset uint16) Address {
	return (Address(seg) << 16) | Address(offset)
}

func (a Address) String() string {
	return fmt.Sprintf("0x%X:0x%X", a.Segment(), a.Offset())
}

func (a Address) Segment() uint16 {
	return uint16(a >> 16)
}

func (a Address) Offset() uint16 {
	return uint16(a & 0xFFFF)
}

func (a Address) Pointer() Pointer {
	return NewPointer(a.Segment(), a.Offset())
}

func (a Address) AddInt(i int) Address {
	return (Address(a) & 0xFFFF0000) | Address(a.Offset()+uint16(i))
}

type Pointer uint32

func NewPointer(seg, offset uint16) Pointer {
	return (Pointer(seg)*0x10 + Pointer(offset)) & 0xFFFFF
}

func (p Pointer) String() string {
	return fmt.Sprintf("0x%X", uint32(p))
}

type Memory interface {
	ReadByte(addr Pointer) byte
	WriteByte(addr Pointer, data byte)
}

type IO interface {
	In(port uint16) byte
	Out(port uint16, data byte)
}

type DummyIO struct{}

func (m *DummyIO) In(port uint16) byte {
	log.Printf("reading unmapped IO port: 0x%X", port)
	return 0xFF
}

func (m *DummyIO) Out(port uint16, data byte) {
	log.Printf("writing unmapped IO port: 0x%X", port)
}

type DummyMemory struct{}

func (m *DummyMemory) ReadByte(addr Pointer) byte {
	log.Printf("reading unmapped memory: 0x%X", addr)
	return 0xFF
}

func (m *DummyMemory) WriteByte(addr Pointer, data byte) {
	log.Printf("writing unmapped memory: 0x%X", addr)
}
