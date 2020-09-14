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

package cgatext

import (
	"log"
	"math/rand"
	"os"
	"sync"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/gdamore/tcell"
)

const (
	memorySize = 0x4000
	memoryBase = 0xB8000
)

var cgaPalette = [16]tcell.Color{
	tcell.ColorBlack,
	tcell.ColorNavy,
	tcell.ColorGreen,
	tcell.ColorTeal,
	tcell.ColorMaroon,
	tcell.ColorPurple,
	tcell.ColorOlive,
	tcell.ColorSilver,
	tcell.ColorGray,
	tcell.ColorBlue,
	tcell.ColorLime,
	tcell.ColorAqua,
	tcell.ColorRed,
	tcell.ColorFuchsia,
	tcell.ColorYellow,
	tcell.ColorWhite,
}

type (
	redrawEvent struct{}
	quitEvent   struct{}
)

type consoleCursor struct {
	update, visible bool
	x, y            byte
}

type keyboardHandler interface {
	SendKeyEvent(interface{}) error
}

type Device struct {
	lock     sync.RWMutex
	quitChan chan struct{}

	dirtyMemory bool
	mem         [memorySize]byte
	crtReg      [0x100]byte

	crtAddr, modeCtrlReg,
	colorCtrlReg, oldColorCtrlReg,
	refresh byte

	cursorPos uint16
	cursor    consoleCursor

	keyboard keyboardHandler
	screen   tcell.Screen
	p        processor.Processor
}

func (m *Device) Install(p processor.Processor) error {
	m.p = p
	m.cursor.visible = true

	// Scramble memory.
	rand.Read(m.mem[:])

	if err := p.InstallMemoryDevice(m, memoryBase, memoryBase+memorySize); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3B0, 0x3DF); err != nil {
		return err
	}
	return m.startRenderLoop()
}

func (m *Device) Name() string {
	return "CGA textmode compatible device"
}

func (m *Device) Reset() {
	m.lock.Lock()
	defer m.lock.Unlock()

	m.colorCtrlReg = 0x20
	m.modeCtrlReg = 1
	m.cursorPos = 0
	m.cursor = consoleCursor{}
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) Close() error {
	m.screen.PostEventWait(tcell.NewEventInterrupt(quitEvent{}))
	<-m.quitChan
	return nil
}

func (m *Device) createStyleFromAttrib(attr byte) tcell.Style {
	blinkEnabled := m.modeCtrlReg&0x20 != 0
	return tcell.StyleDefault.Blink(blinkEnabled && attr&0x80 != 0).Background(cgaPalette[attr&0x70>>4]).Foreground(cgaPalette[attr&0xF])
}

func toUnicode(ch byte) rune {
	return codePage437[ch]
}

func (m *Device) startRenderLoop() error {
	tcell.SetEncodingFallback(tcell.EncodingFallbackASCII)
	s, err := tcell.NewScreen()
	if err != nil {
		return err
	}
	if err = s.Init(); err != nil {
		return err
	}

	s.ShowCursor(0, 0)
	s.DisableMouse()
	s.Clear()

	m.screen = s
	m.dirtyMemory = true
	m.quitChan = make(chan struct{})

	redrawTicker := time.NewTicker(time.Second / 30)
	go func() {
		for {
			ev := s.PollEvent()
			switch ev := ev.(type) {
			case *tcell.EventKey:
				switch ev.Key() {
				case tcell.KeyF12:
					go func() {
						m.Close()
						os.Exit(0)
					}()
				default:
					if m.keyboard == nil {
						if kb, ok := m.p.GetMappedIODevice(0x60).(keyboardHandler); ok {
							m.keyboard = kb
						} else {
							log.Panic("Could not find compatible keyboard handler!")
						}
					}
					m.keyboard.SendKeyEvent(ev)
				}
			case *tcell.EventResize:
				s.Sync()
				m.lock.Lock()
				m.dirtyMemory = true
				m.lock.Unlock()
			case *tcell.EventInterrupt:
				switch ev.Data().(type) {
				case quitEvent:
					s.Fini()
					redrawTicker.Stop()
					close(m.quitChan)
					return
				case redrawEvent:
					m.lock.Lock()

					if bg := m.colorCtrlReg & 0xF; bg != m.oldColorCtrlReg {
						m.oldColorCtrlReg = bg
						s.Fill(' ', tcell.StyleDefault.Background(cgaPalette[bg]))
					}

					const numColumns = 80
					for y := 0; y < 25; y++ {
						for x := 0; x < numColumns; x++ {
							offset := y*numColumns*2 + x*2
							s.SetCell(x, y, m.createStyleFromAttrib(m.mem[offset+1]), toUnicode(m.mem[offset]))
						}
					}

					if m.cursor.update {
						m.cursor.update = false
						if m.cursor.visible {
							s.ShowCursor(int(m.cursor.x), int(m.cursor.y))
						} else {
							s.HideCursor()
						}
					}

					m.dirtyMemory = false
					m.lock.Unlock()
					s.Show()
				}
			}
		}
	}()

	go func() {
		for range redrawTicker.C {
			m.lock.RLock()
			dirty := m.dirtyMemory
			m.lock.RUnlock()
			if dirty {
				s.PostEvent(tcell.NewEventInterrupt(redrawEvent{}))
			}
		}
	}()

	return nil
}

func (m *Device) In(port uint16) byte {
	m.lock.Lock()
	defer m.lock.Unlock()

	switch port {
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		return m.crtReg[m.crtAddr]
	case 0x3DA:
		m.refresh ^= 0x9
		return m.refresh
	case 0x3B9:
		return m.colorCtrlReg
	}
	return 0
}

func (m *Device) Out(port uint16, data byte) {
	m.lock.Lock()
	defer m.lock.Unlock()

	switch port {
	case 0x3D0, 0x3D2, 0x3D4, 0x3D6:
		m.crtAddr = data
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		m.crtReg[m.crtAddr] = data
		switch m.crtAddr {
		case 0xA:
			m.cursor.update = true
			m.cursor.visible = data&0x20 == 0
		case 0xE:
			m.cursor.update = true
			m.cursorPos = (m.cursorPos & 0x00FF) | (uint16(data) << 8)
		case 0xF:
			m.cursor.update = true
			m.cursorPos = (m.cursorPos & 0xFF00) | uint16(data)
		}

		m.cursor.x = byte(m.cursorPos % 80)
		m.cursor.y = byte(m.cursorPos / 80)
	case 0x3D8:
		m.modeCtrlReg = data
	case 0x3B9:
		m.colorCtrlReg = data
	}
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	m.lock.RLock()
	v := m.mem[(addr-memoryBase)&0x3FFF]
	m.lock.RUnlock()
	return v
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	m.lock.Lock()
	m.dirtyMemory = true
	m.mem[(addr-memoryBase)&0x3FFF] = data
	m.lock.Unlock()
}
