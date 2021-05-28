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

package cga

import (
	"flag"
	"fmt"
	"math/rand"
	"sync"
	"sync/atomic"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/platform"
	"github.com/andreas-jonsson/virtualxt/platform/dialog"
)

const (
	memorySize     = 0x10000
	memoryStart    = 0xB0000
	cgaBase        = 0x8000
	scanlineTiming = 31469
)

var applicationStart = time.Now()

var cgaColor = []uint32{
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xAA5500,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF,
}

type Device struct {
	lock     sync.RWMutex
	quitChan chan struct{}

	dirtyMemory int32
	mem         [memorySize]byte
	crtReg      [0x100]byte

	crtAddr, modeCtrlReg,
	colorCtrlReg, statusReg byte

	hgcEnable byte
	hgcBase   int
	hgcMode   bool

	lastScanline    int64
	currentScanline int

	cursorVisible,
	prevCursorState bool
	cursorPosition uint16
	surface        []byte

	windowTitleTicker  *time.Ticker
	atomicCycleCounter int32

	p processor.Processor
}

func (m *Device) Install(p processor.Processor) error {
	m.p = p
	m.windowTitleTicker = time.NewTicker(time.Second)
	m.quitChan = make(chan struct{})

	// Scramble memory.
	rand.Read(m.mem[:])

	if err := p.InstallMemoryDevice(m, memoryStart, (memoryStart+memorySize)-1); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3D0, 0x3DF); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3B0, 0x3BF); err != nil {
		return err
	}

	m.surface = make([]byte, 720*350*4)
	go m.renderLoop()
	return nil
}

func (m *Device) Name() string {
	return "CGA/HGC Compatible Device"
}

func (m *Device) Reset() {
	m.lock.Lock()
	m.lastScanline = time.Now().UnixNano()
	m.currentScanline = 0
	m.colorCtrlReg = 0x20
	m.modeCtrlReg = 1
	m.statusReg = 0
	m.hgcEnable = 0
	m.hgcBase = 0
	m.hgcMode = false
	m.cursorVisible = true
	m.cursorPosition = 0
	m.lock.Unlock()
}

func (m *Device) Step(cycles int) error {
	atomic.AddInt32(&m.atomicCycleCounter, int32(cycles))

	t := time.Now().UnixNano()
	d := t - m.lastScanline
	scanlines := d / scanlineTiming

	if scanlines > 0 {
		offset := d % scanlineTiming
		m.lastScanline = t - offset

		if m.currentScanline = (m.currentScanline + int(scanlines)) % 525; m.currentScanline > 479 {
			m.statusReg = 88
		} else {
			m.statusReg = 0
		}
		m.statusReg |= 1
	}
	return nil
}

func (m *Device) Close() error {
	m.quitChan <- struct{}{}
	<-m.quitChan
	return nil
}

func blit32(pixels []byte, offset int, color uint32) {
	pixels[offset] = byte((color & 0xFF0000) >> 16)
	pixels[offset+1] = byte((color & 0x00FF00) >> 8)
	pixels[offset+2] = byte(color & 0x0000FF)
	pixels[offset+3] = 0xFF
}

func blinkTick() bool {
	return ((time.Since(applicationStart)/time.Millisecond)/500)%2 == 0
}

func (m *Device) blitChar(ch, attrib byte, x, y int) {
	pixels := m.surface
	bgColorIndex := (attrib & 0x70) >> 4
	fgColorIndex := attrib & 0xF

	if attrib&0x80 != 0 {
		if m.modeCtrlReg&0x20 != 0 {
			if blinkTick() {
				fgColorIndex = bgColorIndex
			}
		} else {
			// High intensity!
			bgColorIndex += 8
		}
	}

	bgColor := cgaColor[bgColorIndex]
	fgColor := cgaColor[fgColorIndex]

	charWidth := 1
	if m.modeCtrlReg&1 == 0 {
		charWidth = 2
	}

	for i := 0; i < 8; i++ {
		glyphLine := cgaFont[int(ch)*8+i]
		for j := byte(0); j < 8; j++ {
			mask := byte(0x80 >> j)
			col := fgColor
			if glyphLine&mask == 0 {
				col = bgColor
			}
			offset := (640*(y+i) + x*charWidth + int(j)*charWidth) * 4
			blit32(pixels, offset, col)
			if charWidth == 2 { // 40 columns?
				blit32(pixels, offset+4, col)
			}
		}
	}
}

func (m *Device) renderLoop() {
	p := platform.Instance
	textFlag := flag.Lookup("text")
	cliMode := textFlag != nil && textFlag.Value.(flag.Getter).Get().(bool)

	ticker := time.NewTicker(time.Second / 30)
	defer ticker.Stop()

	for {
		select {
		case <-m.quitChan:
			close(m.quitChan)
			return
		case <-ticker.C:
			select {
			case <-m.windowTitleTicker.C:
				hlp := " (Press F12 for menu)"
				if dialog.MainMenuWasOpen() || time.Since(applicationStart) > time.Second*10 {
					hlp = ""
				}
				numCycles := float64(atomic.SwapInt32(&m.atomicCycleCounter, 0))
				p.SetTitle(fmt.Sprintf("VirtualXT - %.2f MIPS%s", numCycles/1000000, hlp))
			default:
			}

			blink := blinkTick()
			dirtyMemory := atomic.LoadInt32(&m.dirtyMemory) != 0

			if dirtyMemory || m.prevCursorState != blink {
				m.lock.RLock()
				atomic.StoreInt32(&m.dirtyMemory, 0)
				m.prevCursorState = blink

				numCol := 80
				if m.modeCtrlReg&1 == 0 {
					numCol = 40
				}

				dst := m.surface
				backgroundColorIndex := m.colorCtrlReg & 0xF
				backgroundColor := cgaColor[backgroundColorIndex]
				bgRComponent, bgGComponent, bgBComponent := byte(backgroundColor&0xFF0000), byte(backgroundColor&0x00FF00), byte(backgroundColor&0x0000FF)

				// In HGC graphics mode?
				if m.hgcMode {
					for y := 0; y < 348; y++ {
						for x := 0; x < 720; x++ {
							addr := ((y & 3) << 13) + (y>>2)*90 + (x >> 3)
							pixel := (m.mem[m.hgcBase+addr] >> (7 - (uint(x) & 7))) & 1
							col := cgaColor[pixel*15]
							offset := (y*720 + x) * 4
							blit32(dst, offset, col)
						}
					}

					for i := 720 * 348; i < 720*350; i++ {
						blit32(dst, i*4, cgaColor[0])
					}

					m.lock.RUnlock()
					p.RenderGraphics(dst, 720, 350, bgRComponent, bgGComponent, bgBComponent)
				} else if m.modeCtrlReg&2 != 0 { // In CGA graphics mode?
					// Is in high-resolution mode?
					if m.modeCtrlReg&0x10 != 0 {
						for y := 0; y < 200; y++ {
							for x := 0; x < 640; x++ {
								addr := (y>>1)*80 + (y&1)*8192 + (x >> 3)
								pixel := (m.mem[cgaBase+addr] >> (7 - (uint(x) & 7))) & 1
								col := cgaColor[pixel*15]
								offset := (y*640 + x) * 4
								blit32(dst, offset, col)
							}
						}
					} else {
						palette := (m.colorCtrlReg >> 5) & 1
						intensity := ((m.colorCtrlReg >> 4) & 1) << 3

						for y := 0; y < 200; y++ {
							for x := 0; x < 320; x++ {
								addr := (y>>1)*80 + (y&1)*8192 + (x >> 2)
								pixel := m.mem[cgaBase+addr]

								switch x & 3 {
								case 0:
									pixel = (pixel >> 6) & 3
								case 1:
									pixel = (pixel >> 4) & 3
								case 2:
									pixel = (pixel >> 2) & 3
								case 3:
									pixel = pixel & 3
								}

								col := backgroundColor
								if pixel != 0 {
									col = cgaColor[pixel*2+palette+intensity]
								}

								offset := (y*640 + x*2) * 4
								blit32(dst, offset, col)
								blit32(dst, offset+4, col)
							}
						}
					}

					m.lock.RUnlock()
					p.RenderGraphics(dst[:640*200*4], 640, 200, bgRComponent, bgGComponent, bgBComponent)
				} else if cliMode {
					if dirtyMemory {
						cx, cy := -1, -1
						if m.cursorVisible {
							cx = int(m.cursorPosition) % numCol
							cy = int(m.cursorPosition) / numCol
						}

						// We need to render before unlock.
						p.RenderText(m.mem[cgaBase:cgaBase+numCol*25*2], m.modeCtrlReg&0x20 != 0, int(backgroundColorIndex), cx, cy)
					}
					m.lock.RUnlock()
				} else {
					videoPage := (int(m.crtReg[0xC]) << 8) + int(m.crtReg[0xD])
					numChar := numCol * 25

					for i := 0; i < numChar*2; i += 2 {
						ch := m.mem[cgaBase+videoPage+i]
						idx := i / 2
						m.blitChar(ch, m.mem[cgaBase+videoPage+i+1], (idx%numCol)*8, (idx/numCol)*8)
					}

					if blink && m.cursorVisible {
						x := int(m.cursorPosition) % numCol
						y := int(m.cursorPosition) / numCol
						if x < 80 && y < 25 {
							attr := (m.mem[cgaBase+videoPage+(numCol*2*y+x*2+1)] & 0x70) | 0xF
							m.blitChar('_', attr, x*8, y*8)
						}
					}

					m.lock.RUnlock()
					p.RenderGraphics(dst[:640*200*4], 640, 200, bgRComponent, bgGComponent, bgBComponent)
				}
			}
		}
	}
}

func (m *Device) In(port uint16) byte {
	m.lock.RLock()
	defer m.lock.RUnlock()

	switch port {
	case 0x3B1, 0x3B3, 0x3B5, 0x3B7:
		fallthrough
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		return m.crtReg[m.crtAddr]
	case 0x3D9:
		return m.colorCtrlReg
	case 0x3BA:
		if !m.hgcMode {
			return 0xFF
		}
	case 0x3DA:
		if m.hgcMode {
			return 0xFF
		}
	default:
		return 0
	}

	status := m.statusReg
	m.statusReg &= 0xFE
	return status
}

func (m *Device) Out(port uint16, data byte) {
	m.lock.Lock()

	// We likely need to redraw the screen.
	atomic.StoreInt32(&m.dirtyMemory, 1)

	switch port {
	case 0x3B0, 0x3B2, 0x3B4, 0x3B6:
		fallthrough
	case 0x3D0, 0x3D2, 0x3D4, 0x3D6:
		m.crtAddr = data
	case 0x3B1, 0x3B3, 0x3B5, 0x3B7:
		fallthrough
	case 0x3D1, 0x3D3, 0x3D5, 0x3D7:
		m.crtReg[m.crtAddr] = data
		switch m.crtAddr {
		case 0xA:
			m.cursorVisible = data&0x20 == 0
		case 0xE:
			m.cursorPosition = (m.cursorPosition & 0x00FF) | (uint16(data) << 8)
		case 0xF:
			m.cursorPosition = (m.cursorPosition & 0xFF00) | uint16(data)
		}
	case 0x3B8: // Bit 7 is HGC page and bit 1 is gfx select
		m.hgcMode = (m.hgcEnable&1)&((data&2)>>1) != 0
		m.hgcBase = 0
		if (m.hgcEnable>>1)&(data>>7) != 0 {
			m.hgcBase = cgaBase
		}
	case 0x3D8:
		m.modeCtrlReg = data
	case 0x3D9:
		m.colorCtrlReg = data
	case 0x3BF: // Enable HGC
		// Set bit 0 to enable bit 1 of 3B8.
		// Set bit 1 to enable bit 7 of 3B8 and the second 32k of RAM ("Full" mode).
		m.hgcEnable = data & 3
	}

	m.lock.Unlock()
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	m.lock.RLock()
	v := m.mem[addr-memoryStart]
	m.lock.RUnlock()
	return v
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	m.lock.Lock()
	atomic.StoreInt32(&m.dirtyMemory, 1)
	m.mem[addr-memoryStart] = data
	m.lock.Unlock()
}
