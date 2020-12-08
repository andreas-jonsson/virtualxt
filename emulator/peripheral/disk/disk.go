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

package disk

import (
	"errors"
	"io"
	"log"
	"sync"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

type diskDrive struct {
	rws           io.ReadWriteSeeker
	fileSize      uint32
	present, isHD bool

	cylinders, sectors, heads uint16
}

type Device struct {
	BootDrive byte

	cpu    processor.Processor
	lock   sync.Mutex
	buffer [512]byte
	numHD  byte

	disks    [0x100]diskDrive
	lookupAH [0x100]byte
	lookupCF [0x100]bool
}

func (m *Device) Install(p processor.Processor) error {
	m.cpu = p
	// IO 0xB0, 0xB1 to interrupt 0x19, 0x13
	return p.InstallIODeviceAt(m, 0xB0, 0xB1)
}

func (m *Device) Name() string {
	return "Disk Controller"
}

func (m *Device) Reset() {
	m.lock.Lock()
	defer m.lock.Unlock()

	m.cpu.WriteByte(memory.NewPointer(0x40, 0x75), m.numHD)

	for i := range m.lookupAH {
		m.lookupAH[i] = 0
	}
	for i := range m.lookupCF {
		m.lookupCF[i] = false
	}
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) Eject(dnum byte) (io.ReadWriteSeeker, error) {
	m.lock.Lock()
	defer m.lock.Unlock()

	d := &m.disks[dnum]
	if !d.present {
		return nil, errors.New("no disk")
	}
	if d.isHD {
		m.numHD--
	}
	d.present = false
	return d.rws, nil
}

func (m *Device) Replace(dnum byte, disk io.ReadWriteSeeker) error {
	m.Eject(dnum)
	return m.Insert(dnum, disk)
}

func (m *Device) Insert(dnum byte, disk io.ReadWriteSeeker) error {
	m.lock.Lock()
	defer m.lock.Unlock()

	d := &m.disks[dnum]
	if d.present {
		return errors.New("has disk")
	}

	d.rws = disk
	sz, err := d.rws.Seek(0, io.SeekEnd)
	if err != nil {
		return err
	}
	d.fileSize = uint32(sz)
	if _, err := d.rws.Seek(0, io.SeekStart); err != nil {
		return err
	}

	if d.isHD = dnum >= 0x80; d.isHD {
		d.heads = 16
		d.sectors = 63
		d.cylinders = uint16(d.fileSize / (uint32(d.sectors) * uint32(d.heads) * 512))
		m.numHD++
	} else {
		switch {
		case d.fileSize <= 1228800:
			d.cylinders = 80
			d.heads = 2
			d.sectors = 15
		case d.fileSize <= 737280:
			d.cylinders = 80
			d.heads = 2
			d.sectors = 9
		case d.fileSize <= 368640:
			d.cylinders = 40
			d.heads = 2
			d.sectors = 9
		case d.fileSize <= 163840:
			d.cylinders = 40
			d.heads = 1
			d.sectors = 8
		default:
			d.cylinders = 80
			d.heads = 2
			d.sectors = 18
		}
	}

	d.present = true
	return nil
}

func (m *Device) bootstrap() {
	d := &m.disks[m.BootDrive]
	if !d.present {
		log.Panic("No bootdrive!")
	}

	r := m.cpu.GetRegisters()
	r.SetDL(m.BootDrive)
	r.SetAL(m.executeOperation(true, d, memory.NewAddress(0x0, 0x7C00), 0, 1, 0, 1))
}

func (m *Device) executeOperation(readOp bool, disc *diskDrive, dst memory.Address, cylinders uint16, sectors, heads, count byte) byte {
	if sectors == 0 {
		return 0
	}

	lba := (int64(cylinders)*int64(disc.heads)+int64(heads))*int64(disc.sectors) + int64(sectors) - 1
	if _, err := disc.rws.Seek(lba*512, io.SeekStart); err != nil {
		return 0
	}

	var numSectors byte
	for numSectors < count {
		if readOp {
			if _, err := io.ReadFull(disc.rws, m.buffer[:]); err != nil {
				break
			}
			for _, v := range m.buffer {
				m.cpu.WriteByte(dst.Pointer(), v)
				dst = dst.AddInt(1)
			}
		} else {
			for i := range m.buffer {
				m.buffer[i] = m.cpu.ReadByte(dst.Pointer())
				dst = dst.AddInt(1)
			}
			if n, err := disc.rws.Write(m.buffer[:]); n != 512 || err != nil {
				break
			}
		}
		numSectors++
	}
	return numSectors
}

func (m *Device) executeAndSet(readOp bool) {
	r := m.cpu.GetRegisters()
	if d := &m.disks[r.DL()]; !d.present {
		r.SetAH(1)
		r.CF = true
	} else {
		r.SetAL(m.executeOperation(readOp, d, memory.NewAddress(r.ES, r.BX), uint16(r.CH())+uint16(r.CL()/64)*256, r.CL()&0x3F, r.DH(), r.AL()))
		r.SetAH(0)
		r.CF = false
	}
}

func (m *Device) In(uint16) byte {
	return 0xFF
}

func (m *Device) Out(port uint16, _ byte) {
	m.lock.Lock()
	defer m.lock.Unlock()

	switch port {
	case 0xB0:
		m.bootstrap()
	case 0xB1:
		r := m.cpu.GetRegisters()
		ah, dl := r.AH(), r.DL()

		switch ah {
		case 0: // Reset
			r.SetAH(0)
			r.CF = false
		case 1: // Return status
			r.SetAH(m.lookupAH[dl])
			r.CF = m.lookupCF[dl]
			return
		case 2: // Read sector
			m.executeAndSet(true)
		case 3: // Write sector
			m.executeAndSet(false)
		case 4, 5: // Format track
			r.CF = false
			r.SetAH(0)
		case 8: // Drive parameters
			if d := &m.disks[dl]; !d.present {
				r.CF = true
				r.SetAH(0xAA)
			} else {
				r.CF = false
				r.SetAH(0)
				r.SetCH(byte(d.cylinders - 1))
				r.SetCL(byte((d.sectors & 0x3F) + (d.cylinders/256)*64))
				r.SetDH(byte(d.heads - 1))
				if dl < 0x80 {
					r.SetBL(4)
					r.SetDL(2)
				} else {
					r.SetDL(m.numHD)
				}
			}
		default:
			r.CF = true
		}

		ah, dl = r.AH(), r.DL()
		if dl&0x80 != 0 {
			m.cpu.WriteByte(memory.NewPointer(0x40, 0x74), ah)
		}

		m.lookupAH[dl] = ah
		m.lookupCF[dl] = r.CF
	default:
	}
}
