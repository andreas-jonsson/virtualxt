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

/*
References:
	https://wiki.osdev.org/Programmable_Interval_Timer
	fake86's - i8253.c
*/

package pit

import (
	"log"
	"time"

	"github.com/andreas-jonsson/i8088-core/emulator/processor"
)

const (
	modeLatchCount = iota
	modeLowByte
	modeHighByte
	modeToggle
)

type pitChannel struct {
	enabled, toggle bool
	frequency       float64
	effective       uint32
	counter, data   uint16
	mode            byte
}

type Device struct {
	pic                         processor.InterruptController
	channels                    [3]pitChannel
	ticks, deviceTicks, tickGap int64
}

func (m *Device) Install(p processor.Processor) error {
	m.pic = p.GetInterruptController()
	return p.InstallIODevice(m, 0x40, 0x43)
}

func (m *Device) Name() string {
	return "Programmable Interval Timer (Intel 8253)"
}

func (m *Device) Reset() {
	*m = Device{pic: m.pic}
}

const hostFrequency int64 = 1000000

func (m *Device) Step(int) error {
	ticks := time.Now().UnixNano() / 1000
	if m.channels[0].enabled && ticks >= (m.ticks+m.tickGap) {
		m.ticks = ticks
		m.pic.IRQ(0)
	}

	tickGap := hostFrequency / 119318
	if ticks >= (m.deviceTicks + tickGap) {
		for _, ch := range m.channels {
			if ch.enabled {
				if ch.counter < 10 {
					ch.counter = ch.data
				} else {
					ch.counter -= 10
				}
			}
		}
		m.deviceTicks = ticks
	}
	return nil
}

func (m *Device) In(port uint16) byte {
	if port == 0x43 {
		return 0
	}

	var ret uint16
	ch := &m.channels[port&3]

	if ch.mode == modeLatchCount || ch.mode == modeLowByte || (ch.mode == modeToggle && !ch.toggle) {
		ret = ch.counter & 0xFF
	} else if ch.mode == modeHighByte || (ch.mode == modeToggle && ch.toggle) {
		ret = ch.counter >> 8
	} else {
		log.Print("PIT mode not handled!")
	}

	if ch.mode == modeLatchCount || ch.mode == modeToggle {
		ch.toggle = !ch.toggle
	}
	return byte(ret)
}

func (m *Device) Out(port uint16, data byte) {
	switch port {
	case 0x40, 0x41, 0x42:
		ch := &m.channels[port&3]
		ch.enabled = true
		data16 := uint16(data)

		if ch.mode == modeLowByte || (ch.mode == modeToggle && !ch.toggle) {
			ch.data = (ch.data & 0xFF00) | data16
		} else if ch.mode == modeHighByte || (ch.mode == modeToggle && ch.toggle) {
			ch.data = (ch.data & 0x00FF) | (data16 << 8)
		} else {
			log.Print("PIT mode not handled!")
		}

		if ch.data == 0 {
			ch.effective = 65536
		} else {
			ch.effective = uint32(ch.data)
		}

		if ch.mode == modeToggle {
			ch.toggle = !ch.toggle
		}

		m.tickGap = int64(float64(hostFrequency) / float64(1193182.0/float64(ch.effective)))
		ch.frequency = float64(uint32((1193182.0/float64(ch.effective))*1000.0)) / 1000.0
	case 0x43: // Mode/Command register.
		ch := &m.channels[data>>6]
		if ch.mode = byte((data >> 4) & 3); ch.mode == modeToggle {
			ch.toggle = false
		}
	}
}
