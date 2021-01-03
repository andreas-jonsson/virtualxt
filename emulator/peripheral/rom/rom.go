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
