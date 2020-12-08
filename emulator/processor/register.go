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

package processor

type Registers struct {
	AX, CX, DX, BX,
	SP, BP, SI, DI,
	ES, CS, SS, DS, IP uint16

	CF, PF, AF, ZF,
	SF, TF, IF, DF, OF bool

	Debug bool
}

func (r *Registers) AL() byte {
	return byte(r.AX & 0xFF)
}

func (r *Registers) AH() byte {
	return byte(r.AX >> 8)
}

func (r *Registers) SetAL(v byte) {
	r.AX = r.AX&0xFF00 | uint16(v)
}

func (r *Registers) SetAH(v byte) {
	r.AX = r.AX&0xFF | uint16(v)<<8
}

func (r *Registers) BL() byte {
	return byte(r.BX & 0xFF)
}

func (r *Registers) BH() byte {
	return byte(r.BX >> 8)
}

func (r *Registers) SetBL(v byte) {
	r.BX = r.BX&0xFF00 | uint16(v)
}

func (r *Registers) SetBH(v byte) {
	r.BX = r.BX&0xFF | uint16(v)<<8
}

func (r *Registers) CL() byte {
	return byte(r.CX & 0xFF)
}

func (r *Registers) CH() byte {
	return byte(r.CX >> 8)
}

func (r *Registers) SetCL(v byte) {
	r.CX = r.CX&0xFF00 | uint16(v)
}

func (r *Registers) SetCH(v byte) {
	r.CX = r.CX&0xFF | uint16(v)<<8
}

func (r *Registers) DL() byte {
	return byte(r.DX & 0xFF)
}

func (r *Registers) DH() byte {
	return byte(r.DX >> 8)
}

func (r *Registers) SetDL(v byte) {
	r.DX = r.DX&0xFF00 | uint16(v)
}

func (r *Registers) SetDH(v byte) {
	r.DX = r.DX&0xFF | uint16(v)<<8
}
