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
