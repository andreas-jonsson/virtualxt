// +build validator

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

package validator

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"math"
	"os"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

const Enabled = true

const (
	queueSize  = 1024            // 1KB
	bufferSize = 0x100000 * 1024 // 1GB
)

var outputFile string

var (
	inScope      bool
	currentEvent Event
	outputChan   chan Event
	quitChan     chan struct{}
)

func Initialize(output string) {
	if outputFile = output; output == "" {
		return
	}

	outputChan = make(chan Event, queueSize)
	quitChan = make(chan struct{})

	fp, err := os.Create(outputFile)
	if err != nil {
		log.Panic(err)
	}

	go func() {
		defer func() { quitChan <- struct{}{} }()
		defer fp.Close()

		var buffer bytes.Buffer
		enc := json.NewEncoder(&buffer)

		for ev := range outputChan {
			if err := enc.Encode(ev); err != nil {
				log.Print(err)
				return
			}
			if buffer.Len() >= bufferSize {
				log.Print("Flush validation events!")
				if _, err := io.Copy(fp, &buffer); err != nil {
					log.Print(err)
					return
				}
			}
		}
	}()
}

func Begin(opcode byte, regs processor.Registers) {
	if outputFile == "" {
		return
	}

	inScope = true
	currentEvent = EmptyEvent
	currentEvent.Opcode = opcode
	currentEvent.Regs[0] = regs

}

func End(regs processor.Registers) {
	if !inScope {
		return
	}

	inScope = false
	currentEvent.Regs[1] = regs
	outputChan <- currentEvent
}

func Discard() {
	inScope = false
}

func ReadByte(addr uint32, data byte) {
	if !inScope {
		return
	}
	for i, op := range currentEvent.Reads {
		if op.Addr == math.MaxUint32 {
			currentEvent.Reads[i] = MemOp{addr, data}
			return
		}
	}
	log.Panic("Max reads!")
}

func WriteByte(addr uint32, data byte) {
	if !inScope {
		return
	}
	for i, op := range currentEvent.Writes {
		if op.Addr == math.MaxUint32 {
			currentEvent.Writes[i] = MemOp{addr, data}
			return
		}
	}
	log.Panic("Max writes!")
}

func Shutdown() {
	if outputFile == "" {
		return
	}
	close(outputChan)
	<-quitChan
}
