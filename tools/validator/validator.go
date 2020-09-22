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
