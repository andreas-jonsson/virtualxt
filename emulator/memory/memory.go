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
