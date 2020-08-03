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

package pic

import (
	"errors"

	"github.com/andreas-jonsson/i8088-core/emulator/processor"
)

var ErrNoInterrupts = errors.New("no interrupts")

type Device struct {
	maskReg, requestReg, serviceReg, icwStep,
	intOffset, priority, autoEOI, readMode byte
	icw     [5]byte
	ticks   uint32
	enabled bool
}

func (m *Device) Install(p processor.Processor) error {
	return p.InstallIODevice(m, 0x20, 0x21)
}

func (m *Device) Name() string {
	return "Programmable Interrupt Controller (Intel 8259)"
}

func (m *Device) Reset() {
	*m = Device{}
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) GetInterrupt() (int, error) {
	has := m.requestReg & (^m.maskReg)
	if has == 0 {
		return 0, ErrNoInterrupts
	}
	for i := 0; i < 8; i++ {
		if (has>>i)&1 != 0 {
			m.requestReg ^= (1 << i)
			m.serviceReg |= (1 << i)
			return int(m.icw[2]) + i, nil
		}
	}
	return 0, nil
}

func (m *Device) IRQ(n int) {
	m.requestReg |= byte(1 << n)
}

func (m *Device) In(port uint16) byte {
	switch port {
	case 0x20:
		if m.readMode == 0 {
			return m.requestReg
		} else {
			return m.serviceReg
		}
	case 0x21:
		return m.maskReg
	}
	return 0
}

func (m *Device) Out(port uint16, data byte) {
	switch port {
	case 0x20:
		if data&0x10 != 0 {
			m.icwStep = 1
			m.maskReg = 0
			m.icw[m.icwStep] = byte(data)
			m.icwStep++
			return
		}
		if (data & 0x98) == 8 {
			if data&2 != 0 {
				m.readMode = byte(data & 2)
			}
		}
		if data&0x20 != 0 {
			for i := 0; i < 8; i++ {
				if (m.serviceReg>>i)&1 != 0 {
					m.serviceReg ^= (1 << i)
					if (i == 0) && (m.ticks > 0) {
						m.ticks = 0
						m.requestReg |= 1
					}
					return
				}
			}
		}
	case 0x21:
		if m.icwStep == 3 && m.icw[1]&2 != 0 {
			m.icwStep = 4
		}
		if m.icwStep < 5 {
			m.icw[m.icwStep] = byte(data)
			m.icwStep++
			return
		}
		m.maskReg = byte(data)
	}
}
