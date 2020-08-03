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
