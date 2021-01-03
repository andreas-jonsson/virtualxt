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
func Initialize(output *C.char, queueSize, bufferSize C.int) {
	validator.Initialize(C.GoString(output), int(queueSize), int(bufferSize))
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
