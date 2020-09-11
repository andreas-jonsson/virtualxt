// +build network

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
	"log"
	"math"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/google/gopacket/pcap"
)

type Device struct {
	netInterface *pcap.Interface
	handle       *pcap.Handle
}

func (m *Device) Install(p processor.Processor) error {
	devices, err := pcap.FindAllDevs()
	if err != nil {
		return err
	}

	log.Print("Detected network devices:")
	for i := range devices {
		dev := &devices[i]
		log.Printf(" |- %s (%s)", dev.Description, dev.Name)

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

	log.Print("Selected network device: ", m.netInterface.Description)
	m.handle, err = pcap.OpenLive(m.netInterface.Name, int32(math.MaxUint16), true, pcap.BlockForever)
	if err != nil {
		return err
	}
	return nil
}

func (m *Device) Name() string {
	return "Network Adapter"
}

func (m *Device) Reset() {
	m.netInterface = nil
	if m.handle != nil {
		m.handle.Close()
	}
}

func (m *Device) Close() {
	if m.handle != nil {
		m.handle.Close()
	}
}

func (m *Device) Step(cycles int) error {
	return nil
}
