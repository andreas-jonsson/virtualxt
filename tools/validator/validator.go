/*
Copyright (c) 2019-2020 Andreas T Jonsson

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

package main

import (
	"encoding/json"
	"flag"
	"log"
	"os"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
)

var (
	vxtInput = "virtualxt.json"
	refInput = "validator.json"
)

func init() {
	flag.StringVar(&vxtInput, "virtualxt", vxtInput, "VirtualXT CPU")
	flag.StringVar(&refInput, "validation", refInput, "Reference CPU")
}

func main() {
	flag.Parse()
	log.SetFlags(0)

	vxtFp, err := os.Open(vxtInput)
	if err != nil {
		log.Panic(err)
	}
	defer vxtFp.Close()

	refFp, err := os.Open(refInput)
	if err != nil {
		log.Panic(err)
	}
	defer refFp.Close()

	vxtDec := json.NewDecoder(vxtFp)
	refDec := json.NewDecoder(refFp)

	var numEq int
	for i := 0; i < 1000000; i++ {
		var a, b validator.Event
		if err := vxtDec.Decode(&a); err != nil {
			log.Panic(err)
		}

		if err := refDec.Decode(&b); err != nil {
			log.Panic(err)
		}

		if equalOpcodeAndLocation(&a, &b) {
			numEq++
		}
	}
	log.Print("Equal:", numEq)
}

func equalAll(a, b *validator.Event) bool {
	return *a == *b
}

func equalOpcodeAndLocation(a, b *validator.Event) bool {
	ar, br := &a.Regs[0], &b.Regs[0]
	return a.Opcode == b.Opcode && memory.NewPointer(ar.CS, ar.IP) == memory.NewPointer(br.CS, br.IP)
}

func equalOpcodeLocationAndFirstRead(a, b *validator.Event) bool {
	if !equalOpcodeAndLocation(a, b) {
		return false
	}
	return a.Reads[0].Addr == 0xFF || a.Reads[0].Data == b.Reads[0].Data
}

func equalOpcodeAndFirstRead(a, b *validator.Event) bool {
	return a.Opcode == b.Opcode && (a.Reads[0].Addr == 0xFF || a.Reads[0].Data == b.Reads[0].Data)
}

func equalInputData(a, b *validator.Event) bool {
	ar, br := &a.Regs[0], &b.Regs[0]
	for i, read := range a.Reads {
		if read.Data != b.Reads[i].Data {
			return false
		}
	}
	if !equalRegs(ar, br) {
		return false
	}
	if !equalFlags(ar, br) {
		return false
	}
	return a.Opcode == b.Opcode
}

func equalRegs(a, b *processor.Registers) bool {
	return a.AX == b.AX &&
		a.CX == b.CX &&
		a.DX == b.DX &&
		a.BX == b.BX &&
		a.SP == b.SP &&
		a.BP == b.BP &&
		a.SI == b.SI &&
		a.DI == b.DI &&
		a.ES == b.ES &&
		a.SS == b.SS &&
		a.DS == b.DS
}

func equalFlags(a, b *processor.Registers) bool {
	return a.CF == b.CF &&
		a.PF == b.PF &&
		a.AF == b.AF &&
		a.ZF == b.ZF &&
		a.SF == b.SF &&
		a.TF == b.TF &&
		a.IF == b.IF &&
		a.DF == b.DF &&
		a.OF == b.OF
}
