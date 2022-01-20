// +build validator

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

package validator

import (
	"bytes"
	"io"
	"log"
	"math"
	"os"
)

// A type used to describe the current state of the validator
type State = int

const (
	Running State = iota	// The start address was hit, the validator is now running
	Waiting					// The start address is not hit yet
	Finished				// The end address was hit, the validator cannot run anymore
)

var (
	outputFile   string
	inScope      bool
	state        State = Waiting
	startAddr    uint32
	endAddr      uint32
)

var (
	currentEvent Event
	outputChan   chan Event
	quitChan     chan struct{}
)

// Registers indexes
const (
	AX = iota
	CX
	DX
	BX
	SP
	BP
	SI
	DI
	ES
	CS
	SS
	DS
)

func Initialize(output string, validatorStartAddr, validatorEndAddr uint32) {
	if outputFile = output; output == "" {
		return
	} else if validatorStartAddr == 0 {
		// If no start address was specified, we launch the validator
		state = Running
	}

	startAddr = validatorStartAddr
	endAddr = validatorEndAddr
	
	outputChan = make(chan Event, DefaultQueueSize)
	quitChan = make(chan struct{})

	fp, err := os.Create(outputFile)
	if err != nil {
		log.Panic(err)
	}

	go func() {
		var buffer bytes.Buffer

		defer fp.Close()
		defer func() { io.Copy(fp, &buffer); quitChan <- struct{}{} }()

		enc := NewEncoder(&buffer)
		defer enc.Close()

		for ev := range outputChan {
			enc.Encode(ev);
			if buffer.Len() >= DefaultBufferSize {
				log.Print("Flush validation events!")
				if _, err := io.Copy(fp, &buffer); err != nil {
					log.Print(err)
					return
				}
			}
		}
	}()
}

func Begin(opcode byte, opcodeExt byte, ip, flags uint16, regs [12]uint16) {
	if outputFile == "" || state == Finished {
		return
	}

	if state == Waiting {
		if addr := GetAddr(regs[CS], ip); addr == startAddr {
			log.Printf("Validator started at %08X.", addr)
			state = Running
		} else {
			return
		}
	}

	inScope = true
	currentEvent = EmptyEvent
	currentEvent.Opcode = opcode
	currentEvent.OpcodeExt = opcodeExt

	currentEvent.Before.IP = ip
	currentEvent.Before.Flags = flags
	currentEvent.Before.Regs = regs
}

func End(ip, flags uint16, regs [12]uint16) {
	if !inScope || state != Running {
		return
	}

	currentEvent.After.IP = ip
	currentEvent.After.Flags = flags
	currentEvent.After.Regs = regs

	inScope = false
	outputChan <- currentEvent

	if addr := GetAddr(regs[CS], ip); addr == endAddr {
		log.Printf("Validator stopped at %08X.", addr)
		Shutdown()
	}
}

func ReadByte(addr uint32, data byte) {
	if !inScope || state != Running {
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
	if !inScope || state != Running {
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
	if outputFile == "" || state != Running {
		return
	}
	state = Finished
	close(outputChan)
	<-quitChan
}

func Discard() {
	inScope = false
}

// Returns a linear address from a segment and an offset
func GetAddr(cs, ip uint16) uint32 {
	return uint32(cs) << 16 | uint32(ip)
}