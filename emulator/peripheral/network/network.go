// +build pcap

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

package network

import (
	"bytes"
	"flag"
	"log"
	"math"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
	"github.com/andreas-jonsson/virtualxt/platform/dialog"
	"github.com/google/gopacket/pcap"
)

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
	return p.InstallIODeviceAt(m, 0xB2)
}

func (m *Device) Name() string {
	return "Network Adapter"
}

func (m *Device) Reset() {
	m.canRecv = false
}

func (m *Device) Close() error {
	if !enabled {
		return nil
	}

	m.quitChan <- struct{}{}
	<-m.quitChan

	if m.handle != nil {
		m.handle.Close()
	}
	return nil
}

func (m *Device) startCapture() {
	m.packets = make(chan []byte)
	m.quitChan = make(chan struct{})

	go func() {
		for {
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
	}()
}

func (m *Device) Step(cycles int) error {
	if !m.canRecv {
		return nil
	}

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

func (m *Device) In(uint16) byte {
	return 0 // Must return 0 to indicate that we have a network card.
}

func (m *Device) Out(uint16, byte) {
	/*
		This is the API of Fake86's packet driver.

		References:
			http://crynwr.com/packet_driver.html
			http://crynwr.com/drivers/
	*/

	validator.Discard()

	r := m.cpu.GetRegisters()
	switch r.AH() {
	case 0: // Enable packet reception
		m.canRecv = true
	case 1: // Send packet of CX at DS:SI
		m.buffer.Reset()
		for i := 0; i < int(r.CX()); i++ {
			m.buffer.WriteByte(m.cpu.ReadByte(memory.NewAddress(r.DS(), r.SI()).AddInt(i).Pointer()))
		}
		if err := m.handle.WritePacketData(m.buffer.Bytes()); err != nil {
			log.Print(err)
		}
	case 2: // Return packet info (packet buffer in DS:SI, length in CX)
		r.SetDS(0xD000)
		r.SetSI(0x0)
		r.SetCX(uint16(m.pkgLen))
	case 3: // Copy packet to final destination (given in ES:DI)
		for i := 0; i < m.pkgLen; i++ {
			m.cpu.WriteByte(memory.NewAddress(r.ES(), r.DI()).AddInt(i).Pointer(), m.cpu.ReadByte(memory.NewAddress(0xD000, 0).AddInt(i).Pointer()))
		}
	case 4:
		m.canRecv = false
	}
}

var enabled bool

func init() {
	flag.BoolVar(&enabled, "network", enabled, "Enable network support")
}
