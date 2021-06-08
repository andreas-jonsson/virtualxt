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
	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
)

//export Initialize
func Initialize(output *C.char, queueSize, bufferSize C.int) {
	validator.Initialize(C.GoString(output), int(queueSize), int(bufferSize))
}

//export Begin
func Begin(opcode C.uchar) {
	validator.Begin(byte(opcode))
}

//export End
func End() {
	validator.End()
}

//export Discard
func Discard() {
	validator.Discard()
}

//export ReadReg8
func ReadReg8(reg, data byte) {
	validator.ReadReg8(reg, data)
}

//export WriteReg8
func WriteReg8(reg, data byte) {
	validator.WriteReg8(reg, data)
}

//export ReadReg16
func ReadReg16(reg byte, data uint16) {
	validator.ReadReg16(reg, data)
}

//export WriteReg16
func WriteReg16(reg byte, data uint16) {
	validator.WriteReg16(reg, data)
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

func main() {}
