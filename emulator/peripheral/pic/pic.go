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

package pic

import (
	"errors"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
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
