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

/*
#include "registers.h"
*/
import "C"
import (
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
)

//export Initialize
func Initialize(output *C.char) {
	validator.Initialize(C.GoString(output))
}

//export Begin
func Begin(opcode C.uchar, regs *C.struct_Registers) {
	validator.Begin(byte(opcode), cToGoRegs(regs))
}

//export End
func End(regs *C.struct_Registers) {
	validator.End(cToGoRegs(regs))
}

//export Discard
func Discard() {
	validator.Discard()
}

//export ReadByte
func ReadByte(addr C.uint, data C.uchar) {
	validator.ReadByte(uint32(addr), byte(data))
}

//export WriteByte
func WriteByte(addr C.uint, data C.uchar) {
	validator.WriteByte(uint32(addr), byte(data))
}

//export Shutdown
func Shutdown() {
	validator.Shutdown()
}

func cToGoRegs(r *C.struct_Registers) processor.Registers {
	return processor.Registers{
		AX: uint16(r.AX), CX: uint16(r.CX), DX: uint16(r.DX), BX: uint16(r.BX),
		SP: uint16(r.SP), BP: uint16(r.BP), SI: uint16(r.SI), DI: uint16(r.DI),
		ES: uint16(r.ES), CS: uint16(r.CS), SS: uint16(r.SS), DS: uint16(r.DS), IP: uint16(r.IP),

		CF: bool(r.CF), PF: bool(r.PF), AF: bool(r.AF), ZF: bool(r.ZF),
		SF: bool(r.SF), TF: bool(r.TF), IF: bool(r.IF), DF: bool(r.DF), OF: bool(r.OF),
	}
}

func main() {}
