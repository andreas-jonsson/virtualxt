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

package rom

import (
	"io"
	"io/ioutil"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

type Device struct {
	mem []byte

	Base    memory.Pointer
	RomName string
	Reader  io.Reader
}

func (m *Device) Install(p processor.Processor) error {
	var err error
	if m.mem, err = ioutil.ReadAll(m.Reader); err != nil {
		return err
	}
	if m.RomName == "" {
		m.RomName = "ROM"
	}
	return p.InstallMemoryDevice(m, m.Base, m.Base+memory.Pointer(len(m.mem)-1))
}

func (m *Device) Name() string {
	return m.RomName
}

func (m *Device) Reset() {
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	return m.mem[addr-m.Base]
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	//log.Printf("don't write to ROM! %v <- 0x%X", addr, data)
}
