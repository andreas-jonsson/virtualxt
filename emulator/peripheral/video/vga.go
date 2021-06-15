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

package video

import (
	"log"
	"math/rand"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/platform"
)

const (
	planeSize      = 0x10000
	vgaMemorySize  = 0x40000
	vgaMemoryStart = 0xA0000
)

type VGADevice struct {
	CGADevice

	vgaQuitChan chan struct{}
	vgaMode     byte

	vgaPalette          [0x100]uint32
	sequencerController [0x100]byte
	graphicsController  [0x100]byte
	vgaLatch            [4]byte
	ctrlIndex           byte
}

func (m *VGADevice) Install(p processor.Processor) error {
	m.p = p
	m.windowTitleTicker = time.NewTicker(time.Second)
	m.quitChan = make(chan struct{})
	m.vgaQuitChan = make(chan struct{})

	m.mem = make([]byte, vgaMemorySize)
	rand.Read(m.mem[:]) // Scramble memory.
	initializeVGAPalette(m.vgaPalette[:])

	if err := p.InstallMemoryDevice(m, vgaMemoryStart, (memoryStart+memorySize)-1); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3D0, 0x3DF); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3B0, 0x3BF); err != nil {
		return err
	}
	if err := p.InstallIODevice(m, 0x3C0, 0x3CF); err != nil {
		return err
	}
	if err := p.InstallInterruptHandler(m, 0x10); err != nil {
		return err
	}

	m.surface = make([]byte, 720*350*4)
	go m.CGADevice.renderLoop()
	go m.renderLoop()
	return nil
}

func (m *VGADevice) Name() string {
	return "VGA Compatible Device (EXPERIMENTAL)"
}

func (m *VGADevice) Reset() {
	m.setVGAMode(0)
	m.CGADevice.Reset()
}

func (m *VGADevice) Close() error {
	m.vgaQuitChan <- struct{}{}
	<-m.vgaQuitChan
	m.CGADevice.Close()
	return nil
}

func (m *VGADevice) Step(cycles int) error {
	return m.CGADevice.Step(cycles)
}

func (m *VGADevice) setVGAMode(mode byte) {
	m.lock.Lock()
	m.vgaMode = mode
	m.lock.Unlock()
}

func (m *VGADevice) HandleInterrupt(int) error {
	r := m.p.GetRegisters()
	al := r.AL()

	switch r.AH() {
	case 0x0: // Set video mode
		log.Printf("New video mode: 0x%X", al)

		switch al {
		case 0xD: // 320x200 16 Colors (EGA)
		//case 0x12: // 640x480 16 Colors (VGA)
		//case 0x13: // 320x200 256 Colors (VGA)
		default:
			log.Print("Mode is handled by BIOS!")

			m.setVGAMode(0)
			m.setBlit(true)
			return processor.ErrInterruptNotHandled
		}

		m.setVGAMode(al)
		m.setBlit(false)
	case 0x10: // VGA DAC
		switch al {
		case 0x10: // Set single register
			m.vgaPalette[byte(r.BX())] = uint32(r.DH()&0x3F)<<24 | uint32(r.CH()&0x3F)<<16 | uint32(r.CL()&0x3F)<<8 | 0xFF
		case 0x12: // Set multiple registers
			bx, cx := r.BX(), r.CX()
			offset := memory.NewAddress(r.ES(), r.DX())

			for dest := bx; dest < bx+cx; dest++ {
				m.vgaPalette[byte(dest)] = uint32(m.p.ReadByte(offset.Pointer())&0x3F)<<24 |
					uint32(m.p.ReadByte(memory.Pointer(offset.AddInt(1).Pointer()))&0x3F)<<16 |
					uint32(m.p.ReadByte(memory.Pointer(offset.AddInt(2).Pointer()))&0x3F)<<8 |
					0xFF
			}
		}
	//case 0x1A: // Display code
	//	r.SetAL(0x1A)
	//	r.SetBL(0x8)
	default:
		return processor.ErrInterruptNotHandled
	}
	return nil
}

func (m *VGADevice) In(port uint16) byte {
	switch port {
	case 0x3C1: // TODO
		return 0
	case 0x3C5: // Sequence controller data
		return m.sequencerController[m.ctrlIndex]
	case 0x3C7: // DAC state
		return 0
	case 0x3C8: // Palette index
		return 0
	case 0x3C9: // RGB data register
		return 0
	}
	return m.CGADevice.In(port)
}

func (m *VGADevice) Out(port uint16, data byte) {
	switch port {
	case 0x3C0: // TODO
	case 0x3C4: // Sequence controller index
		m.ctrlIndex = data
	case 0x3C5: // Sequence controller data
		m.sequencerController[m.ctrlIndex] = data
	case 0x3C7: // VGA color index
		// TODO
	case 0x3C8: // VGA color index
		// TODO
	case 0x3C9: //RGB data register
	default:
		m.CGADevice.Out(port, data)
	}
}

func (m *VGADevice) readFromLatch() byte {
	v := m.sequencerController[2]
	switch {
	case v&1 != 0:
		return m.vgaLatch[0]
	case v&2 != 0:
		return m.vgaLatch[1]
	case v&4 != 0:
		return m.vgaLatch[2]
	case v&8 != 0:
		return m.vgaLatch[3]
	default:
		log.Print("Invalid plane mask!")
		return 0
	}
}

func (m *VGADevice) ReadByte(addr memory.Pointer) byte {
	m.lock.RLock()
	if m.vgaMode != 0 {
		defer m.lock.RUnlock()

		i := addr - memoryStart
		m.vgaLatch[0] = m.mem[i]
		m.vgaLatch[1] = m.mem[i+planeSize]
		m.vgaLatch[2] = m.mem[i+planeSize*2]
		m.vgaLatch[3] = m.mem[i+planeSize*3]
		return m.readFromLatch()
	}
	m.lock.RUnlock()
	return m.CGADevice.ReadByte(addr)
}

func (m *VGADevice) opWrite(addr memory.Pointer, data, gc, latch byte) {
	ld := m.vgaLatch[latch]
	data = m.vgaOp(data, ld)
	m.mem[addr-memoryStart] = (^gc & data) | (gc & ld)
}

func (m *VGADevice) WriteByte(addr memory.Pointer, data byte) {
	m.lock.Lock()
	if m.vgaMode != 0 {
		defer m.lock.Unlock()

		clamp := func(b, v byte) byte {
			if b != 0 {
				return 0xFF
			}
			return 0
		}

		// Switch on write mode
		switch s := m.sequencerController[2]; m.graphicsController[2] & 3 {
		case 0:
			data = m.vgaShift(data)
			switch {
			case s&1 != 0:
				if m.graphicsController[1]&1 != 0 {
					data = clamp(m.graphicsController[0]&1, data)
				}
				m.opWrite(addr, data, m.graphicsController[8], 0)
			case s&2 != 0:
				if m.graphicsController[1]&2 != 0 {
					data = clamp(m.graphicsController[0]&2, data)
				}
				m.opWrite(addr+planeSize, data, m.graphicsController[8], 1)
			case s&4 != 0:
				if m.graphicsController[1]&4 != 0 {
					data = clamp(m.graphicsController[0]&4, data)
				}
				m.opWrite(addr+planeSize*2, data, m.graphicsController[8], 2)
			case s&8 != 0:
				if m.graphicsController[1]&8 != 0 {
					data = clamp(m.graphicsController[0]&8, data)
				}
				m.opWrite(addr+planeSize*3, data, m.graphicsController[8], 3)
			}
		case 1:
			i := addr - memoryStart
			switch {
			case s&1 != 0:
				m.mem[i] = m.vgaLatch[0]
			case s&2 != 0:
				m.mem[i+planeSize] = m.vgaLatch[1]
			case s&4 != 0:
				m.mem[i+planeSize*2] = m.vgaLatch[2]
			case s&8 != 0:
				m.mem[i+planeSize*3] = m.vgaLatch[3]
			default:
				log.Print("Invalid plane mask!")
			}
		case 2:
			switch {
			case s&1 != 0:
				if m.graphicsController[1]&1 != 0 {
					data = clamp(data&1, data)
				}
				m.opWrite(addr, data, m.graphicsController[8], 0)
			case s&2 != 0:
				if m.graphicsController[1]&2 != 0 {
					data = clamp(data&2, data)
				}
				m.opWrite(addr+planeSize, data, m.graphicsController[8], 1)
			case s&4 != 0:
				if m.graphicsController[1]&4 != 0 {
					data = clamp(data&4, data)
				}
				m.opWrite(addr+planeSize*2, data, m.graphicsController[8], 2)
			case s&8 != 0:
				if m.graphicsController[1]&8 != 0 {
					data = clamp(data&8, data)
				}
				m.opWrite(addr+planeSize*3, data, m.graphicsController[8], 3)
			}
		case 3:
			temp := data & m.graphicsController[8]
			data = m.vgaShift(data)

			switch {
			case s&1 != 0:
				data = clamp(m.graphicsController[0]&1, data)
				m.opWrite(addr, data, temp, 0)
			case s&2 != 0:
				data = clamp(m.graphicsController[0]&2, data)
				m.opWrite(addr+planeSize, data, temp, 1)
			case s&4 != 0:
				data = clamp(m.graphicsController[0]&4, data)
				m.opWrite(addr+planeSize*2, data, temp, 2)
			case s&8 != 0:
				data = clamp(m.graphicsController[0]&8, data)
				m.opWrite(addr+planeSize*3, data, temp, 3)
			}
		}
		return
	}
	m.lock.Unlock()
	m.CGADevice.WriteByte(addr, data)
}

func (m *VGADevice) vgaShift(v byte) byte {
	s := m.graphicsController[3]
	for i := byte(0); i < s&7; i++ {
		v = v>>1 | (v&1)<<7
	}
	return v
}

func (m *VGADevice) vgaOp(v, l byte) byte {
	switch (m.graphicsController[3] >> 3) & 3 {
	case 1:
		return v & l
	case 2:
		return v | l
	case 3:
		return v ^ l
	default:
		return v
	}
}

func (m *VGADevice) renderLoop() {
	p := platform.Instance
	ticker := time.NewTicker(time.Second / 30)
	defer ticker.Stop()

	for {
		select {
		case <-m.vgaQuitChan:
			close(m.vgaQuitChan)
			return
		case <-ticker.C:
			m.lock.RLock()

			dst := m.surface
			switch m.vgaMode {
			case 0xD:
				for y := 0; y < 200; y++ {
					for x := 0; x < 320; x++ {
						addr := y*40 + x>>3
						//addr := (y>>1)*80 + (y&1)*8192 + (x >> 2)
						shift := byte(7 - x&7)

						index := (m.mem[addr] >> shift) & 1
						index += ((m.mem[addr+planeSize] >> shift) & 1) << 1
						index += ((m.mem[addr+planeSize*2] >> shift) & 1) << 2
						index += ((m.mem[addr+planeSize*3] >> shift) & 1) << 3
						color := uint32(index) //m.vgaPalette[index]

						offset := (y*640 + x*2) * 4
						blit32(dst, offset, color)
						blit32(dst, offset+4, color)
					}
				}

				m.lock.RUnlock()
				p.RenderGraphics(dst[:640*200*4], 640, 200, 0, 0, 0)
			default:
				m.lock.RUnlock()
			}
		}
	}
}
