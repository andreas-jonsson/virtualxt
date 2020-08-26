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

package smouse

import (
	"bytes"
	"sync"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

const maxBufferSize = 16

type SerialMouse interface {
	PushEvent(buttons byte, xrel, yrel int8)
}

type Device struct {
	BasePort uint16
	IRQ      int

	lock sync.Mutex

	registers [8]byte
	buffer    bytes.Buffer
	pic       processor.InterruptController
}

func (m *Device) Install(p processor.Processor) error {
	m.pic = p.GetInterruptController()
	return p.InstallIODevice(m, m.BasePort, m.BasePort+7)
}

func (m *Device) Name() string {
	return "Microsoft Serial Mouse"
}

func (m *Device) Reset() {
	m.lock.Lock()
	defer m.lock.Unlock()

	m.buffer.Reset()
	for i := range m.registers {
		m.registers[i] = 0
	}
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) pushData(data byte) {
	ln := m.buffer.Len()
	if ln == maxBufferSize {
		return
	}
	if ln == 0 {
		m.pic.IRQ(m.IRQ)
	}
	m.buffer.WriteByte(data)
}

func (m *Device) PushEvent(buttons byte, xrel, yrel int8) {
	m.lock.Lock()
	defer m.lock.Unlock()

	var upper byte
	if xrel < 0 {
		upper = 0x3
	}
	if yrel < 0 {
		upper |= 0xC
	}

	m.pushData(0x40 | ((buttons & 3) << 4) | upper)
	m.pushData(byte(xrel & 0x3F))
	m.pushData(byte(yrel & 0x3F))
}

func (m *Device) In(port uint16) byte {
	m.lock.Lock()
	defer m.lock.Unlock()

	reg := port & 7
	switch reg {
	case 0: // Serial Data Register
		data, err := m.buffer.ReadByte()
		if err != nil {
			data = 0
		}
		if m.buffer.Len() > 0 {
			m.pic.IRQ(m.IRQ)
		}
		return data
	case 5: // Line Status Register
		if m.buffer.Len() > 0 {
			return 0x61
		}
		return 0x60
	}
	return m.registers[reg]
}

func (m *Device) Out(port uint16, data byte) {
	m.lock.Lock()
	defer m.lock.Unlock()

	reg := port & 7
	rval := m.registers[reg]
	m.registers[reg] = data

	if reg == 4 { // Modem Control Register
		if data&1 != rval&1 {
			m.buffer.Reset()
			m.pushData('M')
		}
	}
}
