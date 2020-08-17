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
	"errors"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

const MaxEvents = 64

type Device struct {
	dataPort, commandPort byte

	state  Scancode
	events chan Scancode
	ticker *time.Ticker
	pic    processor.InterruptController
}

func (m *Device) Install(p processor.Processor) error {
	m.pic = p.GetInterruptController()
	m.ticker = time.NewTicker(time.Millisecond * 10)
	m.events = make(chan Scancode, MaxEvents)

	if err := p.InstallIODeviceAt(m, 0x60, 0x62, 0x64); err != nil {
		return err
	}
	m.startSDLEventLoop()
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

func (m *Device) pushEvent(ev Scancode) error {
	select {
	case m.events <- ev:
		return nil
	default:
		return errors.New("event queue is full")
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

type Scancode byte

const KeyUpMask Scancode = 0x80

const (
	ScanInvalid Scancode = iota
	ScanEscape
	Scan1
	Scan2
	Scan3
	Scan4
	Scan5
	Scan6
	Scan7
	Scan8
	Scan9
	Scan0
	ScanMinus
	ScanEqual
	ScanBackspace
	ScanTab
	ScanQ
	ScanW
	ScanE
	ScanR
	ScanT
	ScanY
	ScanU
	ScanI
	ScanO
	ScanP
	ScanLBracket
	ScanRBracket
	ScanEnter
	ScanControl
	ScanA
	ScanS
	ScanD
	ScanF
	ScanG
	ScanH
	ScanJ
	ScanK
	ScanL
	ScanSemicolon
	ScanQuote
	ScanBackquote
	ScanLShift
	ScanBackslash
	ScanZ
	ScanX
	ScanC
	ScanV
	ScanB
	ScanN
	ScanM
	ScanComma
	ScanPeriod
	ScanSlash
	ScanRShift
	ScanPrint
	ScanAlt
	ScanSpace
	ScanCapslock
	ScanF1
	ScanF2
	ScanF3
	ScanF4
	ScanF5
	ScanF6
	ScanF7
	ScanF8
	ScanF9
	ScanF10
	ScanNumlock
	ScanScrlock
	ScanKPHome
	ScanKPUp
	ScanKPPageup
	ScanKPMinus
	ScanKPLeft
	ScanKP5
	ScanKPRight
	ScanKPPlus
	ScanKPEnd
	ScanKPDown
	ScanKPPagedown
	ScanKPInsert
	ScanKPDelete
)
