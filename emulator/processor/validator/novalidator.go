// +build !validator

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

const Enabled = false

func Initialize(string, int, int) {}
func Begin(byte)                  {}
func End()                        {}
func Discard()                    {}
func ReadReg8(byte, byte)         {}
func WriteReg8(byte, byte)        {}
func ReadReg16(byte, uint16)      {}
func WriteReg16(byte, uint16)     {}
func ReadByte(uint32, byte)       {}
func WriteByte(uint32, byte)      {}
func Shutdown()                   {}
