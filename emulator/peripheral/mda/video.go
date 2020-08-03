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

package mda

import (
	"flag"
	"log"
	"math/rand"
	"os"
	"sync"
	"time"

	"github.com/andreas-jonsson/i8088-core/emulator/memory"
	"github.com/andreas-jonsson/i8088-core/emulator/processor"
	"github.com/gdamore/tcell"
)

const (
	memorySize     = 0x1000
	currentPalette = 0
)

var memoryBase memory.Pointer = 0xB8000

var mdaPalettes = [][4]tcell.Color{
	{tcell.ColorBlack, tcell.ColorGray, tcell.ColorSilver, tcell.ColorWhite},
	{tcell.ColorBlack, tcell.ColorOlive, tcell.ColorGreen, tcell.ColorLightGreen},
}

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
	refresh byte

	cursorPos uint16
	cursor    consoleCursor

	keyboard keyboardHandler
	screen   tcell.Screen
	p        processor.Processor
}

var mdaCompat bool

func init() {
	flag.BoolVar(&mdaCompat, "strict-mda", false, "Strict MDA emulation")
}

func (m *Device) Install(p processor.Processor) error {
	m.p = p
	m.cursor.visible = true

	if mdaCompat {
		memoryBase = 0xB0000
	}

	// Scramble memory.
	rand.Read(m.mem[:])

	if err := p.InstallInterruptHandler(0x10, m); err != nil {
		return err
	}
	if err := p.InstallMemoryDevice(m, memoryBase, memoryBase+memorySize); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3B0, 0x3DF); err != nil {
		return err
	}
	return m.startRenderLoop()
}

func (m *Device) Name() string {
	return "MDA/CGA compatible device"
}

func (m *Device) Reset() {
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
	// Reference: https://www.seasip.info/VintagePC/mda.html

	p := mdaPalettes[currentPalette]
	s := tcell.StyleDefault

	blinkEnabled := m.modeCtrlReg&0x20 != 0

	if !mdaCompat {
		return tcell.StyleDefault.Blink(blinkEnabled && attr&0x80 != 0).Background(cgaPalette[attr&0x70>>4]).Foreground(cgaPalette[attr&0xF])
	}

	switch attr {
	case 0x0, 0x8, 0x80, 0x88:
		return s.Foreground(p[0]).Background(p[0])
	case 0x70:
		return s.Foreground(p[0]).Background(p[2])
	case 0x78:
		return s.Foreground(p[1]).Background(p[2])
	case 0xF0:
		if blinkEnabled {
			return s.Foreground(p[0]).Background(p[2]).Blink(true)
		}
		return s.Foreground(p[0]).Background(p[3])
	case 0xF8:
		if blinkEnabled {
			return s.Foreground(p[1]).Background(p[2]).Blink(true)
		}
		return s.Foreground(p[1]).Background(p[3])
	default:
		s = s.Foreground(p[2]).Background(p[0])
		if attr&8 != 0 {
			s = s.Foreground(p[3])
		}
		return s.Blink(blinkEnabled && attr&0x80 != 0).Underline(attr&7 != 7)
	}
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

					const numColumns = 80
					//reverse := tcell.StyleDefault.Reverse(true)

					for y := 0; y < 25; y++ {
						for x := 0; x < numColumns; x++ {
							offset := y*numColumns*2 + x*2
							s.SetCell(x, y, m.createStyleFromAttrib(m.mem[offset+1]), toUnicode(m.mem[offset]))
						}
						//s.SetCell(numColumns, y, reverse, ' ')
					}
					for x := 0; x <= numColumns; x++ {
						//s.SetCell(x, 25, reverse, ' ')
					}

					if m.cursor.update {
						if !m.cursor.visible {
							s.HideCursor()
						} else {
							s.ShowCursor(int(m.cursor.x), int(m.cursor.y))
						}
						m.cursor.update = false
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

func (m *Device) HandleInterrupt(int) error {
	r := m.p.GetRegisters()
	switch r.AH() {
	case 0:
		videoMode := r.AL() & 0x7F
		log.Printf("Set video mode: 0x%X", videoMode)

		videoMode = 3
		if mdaCompat {
			videoMode = 7
		}

		m.p.WriteByte(memory.NewPointer(0x40, 0x49), videoMode)
		m.p.WriteWord(memory.NewPointer(0x40, 0x4A), 80)

		for i := 0; i < 80*25*2; i += 2 {
			m.mem[i] = 0
			m.mem[i+1] = 7
		}
		return nil
	}
	return processor.ErrInterruptNotHandled
}

func (m *Device) In(port uint16) byte {
	if !mdaCompat {
		addr := memory.NewPointer(0x40, 0x10)
		m.p.WriteWord(addr, (m.p.ReadWord(addr)&0xFFCF)|0x20)
	}

	switch port {
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		if mdaCompat {
			return 0
		}
		fallthrough
	case 0x3B1, 0x3B3, 0x3B5, 0x3B7:
		return m.crtReg[m.crtAddr]
	case 0x3BA, 0x3DA:
		m.refresh ^= 0x9
		return m.refresh
	}
	return 0
}

func (m *Device) Out(port uint16, data byte) {
	switch port {
	case 0x3D0, 0x3D2, 0x3D4, 0x3D6:
		if mdaCompat {
			return
		}
		fallthrough
	case 0x3B0, 0x3B2, 0x3B4, 0x3B6:
		m.crtAddr = data
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		if mdaCompat {
			return
		}
		fallthrough
	case 0x3B1, 0x3B3, 0x3B5, 0x3B7:
		m.lock.Lock()

		m.crtReg[m.crtAddr] = data
		switch m.crtAddr {
		case 0xA:
			m.cursor.update = true
			m.cursor.visible = data&0x20 != 0
		case 0xE:
			m.cursor.update = true
			m.cursorPos = (m.cursorPos & 0x00FF) | (uint16(data) << 8)
		case 0xF:
			m.cursor.update = true
			m.cursorPos = (m.cursorPos & 0xFF00) | uint16(data)
		}

		m.cursor.x = byte(m.cursorPos % 80)
		m.cursor.y = byte(m.cursorPos / 80)

		m.lock.Unlock()
	case 0x3D8:
		if mdaCompat {
			return
		}
		fallthrough
	case 0x3B8:
		m.modeCtrlReg = data
	}
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	m.lock.RLock()
	v := m.mem[(addr-memoryBase)&0xFFF]
	m.lock.RUnlock()
	return v
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	m.lock.Lock()
	m.dirtyMemory = true
	m.mem[(addr-memoryBase)&0xFFF] = data
	m.lock.Unlock()
}
