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

package keyboard

import (
	"log"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/platform"
)

const MaxEvents = 64

type Device struct {
	dataPort, commandPort byte

	state  platform.Scancode
	events chan platform.Scancode
	ticker *time.Ticker
	pic    processor.InterruptController
}

func (m *Device) Install(p processor.Processor) error {
	m.pic = p.GetInterruptController()
	m.ticker = time.NewTicker(time.Millisecond * 10)
	m.events = make(chan platform.Scancode, MaxEvents)

	if err := p.InstallIODeviceAt(m, 0x60, 0x62, 0x64); err != nil {
		return err
	}
	platform.Instance.SetKeyboardHandler(m.eventHandler)
	return nil
}

func (m *Device) Name() string {
	return "Keyboard Controller"
}

func (m *Device) Reset() {
	m.dataPort = 0
	m.commandPort = 0
	for {
		select {
		case <-m.events:
		default:
			return
		}
	}
}

func (m *Device) checkEvents() bool {
	select {
	case <-m.ticker.C:
		select {
		case m.state = <-m.events:
			return true
		default:
		}
	default:
	}
	return false
}

func (m *Device) eventHandler(ev platform.Scancode) {
	select {
	case m.events <- ev:
	default:
		log.Print("Event queue is full!")
	}
}

func (m *Device) Step(int) error {
	if m.checkEvents() {
		m.commandPort |= 2
		m.dataPort = byte(m.state)
		m.pic.IRQ(1)
	}
	return nil
}

func (m *Device) Close() error {
	m.ticker.Stop()
	return nil
}

func (m *Device) In(port uint16) byte {
	switch port {
	case 0x60:
		m.commandPort = 0
		return m.dataPort
	case 0x62:
		return 0
	case 0x64:
		return m.commandPort
	}
	return 0
}

func (m *Device) Out(port uint16, data byte) {
}
