// +build pcap

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

package network

import (
	"bytes"
	"flag"
	"log"
	"math"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/dialog"
	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/google/gopacket/pcap"
)

var enabled bool

type Device struct {
	cpu      processor.Processor
	quitChan chan struct{}

	netInterface *pcap.Interface
	handle       *pcap.Handle

	canRecv bool
	pkgLen  int
	packets chan []byte
	buffer  bytes.Buffer
}

func (m *Device) Install(p processor.Processor) error {
	if !enabled {
		return nil
	}

	m.cpu = p
	devices, err := pcap.FindAllDevs()
	if err != nil {
		dialog.WindowsInstallNpcap()
		return err
	}

	log.Print("Detected network devices:")
	for i := range devices {
		dev := &devices[i]
		log.Printf(" |- %s (%s)", dev.Name, dev.Description)

		var candidate *pcap.Interface
		for _, addr := range dev.Addresses {
			if addr.IP.IsUnspecified() || addr.IP.IsLoopback() {
				candidate = nil
				break
			} else {
				log.Printf(" |  |- %v", addr.IP)
				candidate = dev
			}
		}

		if candidate != nil {
			if m.netInterface == nil {
				m.netInterface = candidate
			}
			log.Printf(" |")
		}
	}

	if m.netInterface == nil {
		log.Print("No network device selected!")
		return nil
	}

	log.Printf("Selected network device: %s (%s)", m.netInterface.Name, m.netInterface.Description)
	m.handle, err = pcap.OpenLive(m.netInterface.Name, int32(math.MaxUint16), true, pcap.BlockForever)
	if err != nil {
		return err
	}
	log.Print("Packet capture is active!")

	m.startCapture()
	return p.InstallInterruptHandler(0xFC, m)
}

func (m *Device) Name() string {
	return "Network Adapter"
}

func (m *Device) Reset() {
	m.canRecv = false
}

func (m *Device) Close() {
	m.quitChan <- struct{}{}
	<-m.quitChan

	if m.handle != nil {
		m.handle.Close()
	}
}

func (m *Device) startCapture() {
	m.packets = make(chan []byte, 1)
	m.quitChan = make(chan struct{})

	go func() {
		for {
			select {
			case <-m.quitChan:
				close(m.quitChan)
				return
			default:
				data, ci, err := m.handle.ReadPacketData()
				if err == nil && ci.Length > 0 {
					select {
					case m.packets <- data[:ci.Length]:
					case <-m.quitChan:
						close(m.quitChan)
						return
					}
				}
			}
		}
	}()
}

var ti = time.NewTicker(1 * time.Millisecond)

func (m *Device) Step(cycles int) error {
	select {
	case data := <-m.packets:
		m.canRecv = false
		m.pkgLen = len(data)

		for i := 0; i < m.pkgLen; i++ {
			m.cpu.WriteByte(memory.NewAddress(0xD000, 0).AddInt(i).Pointer(), data[i])
		}
		m.cpu.GetInterruptController().IRQ(6)
	default:
	}
	return nil
}

func (m *Device) HandleInterrupt(int) error {
	/*
		This is the API of Fake86's packet driver.

		References:
			http://crynwr.com/packet_driver.html
			http://crynwr.com/drivers/
	*/

	r := m.cpu.GetRegisters()
	switch r.AH() {
	case 0: // Enable packet reception
		m.canRecv = true
	case 1: // Send packet of CX at DS:SI
		m.buffer.Reset()
		for i := 0; i < int(r.CX); i++ {
			m.buffer.WriteByte(m.cpu.ReadByte(memory.NewAddress(r.DS, r.SI).AddInt(i).Pointer()))
		}
		if err := m.handle.WritePacketData(m.buffer.Bytes()); err != nil {
			log.Print(err)
		}
	case 2: // Return packet info (packet buffer in DS:SI, length in CX)
		r.DS = 0xD000
		r.SI = 0x0
		r.CX = uint16(m.pkgLen)
	case 3: // Copy packet to final destination (given in ES:DI)
		for i := 0; i < m.pkgLen; i++ {
			m.cpu.WriteByte(memory.NewAddress(r.ES, r.DI).AddInt(i).Pointer(), m.cpu.ReadByte(memory.NewAddress(0xD000, 0).AddInt(i).Pointer()))
		}
	case 4:
		m.canRecv = false
	}
	return nil
}

func init() {
	flag.BoolVar(&enabled, "network", enabled, "Enable network support")
}
