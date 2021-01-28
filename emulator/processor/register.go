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

package processor

import (
	"log"

	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
)

type Registers struct {
	ax, cx, dx, bx,
	sp, bp, si, di,
	es, cs, ss, ds uint16

	IP uint16

	CF, PF, AF, ZF,
	SF, TF, IF, DF, OF bool

	Debug bool
}

func (r *Registers) SegOverridePtr(op byte) *uint16 {
	switch op {
	case 0x26:
		return &r.es
	case 0x2E:
		return &r.cs
	case 0x36:
		return &r.ss
	case 0x3E:
		return &r.ds
	default:
		return nil
	}
}

func (r *Registers) Exchange(op byte) {
	xchg := func(a, b *uint16) {
		tmp := *a
		*a = *b
		*b = tmp
	}

	validator.Discard()

	switch op {
	case 0x91: // XCHG AX,CX
		xchg(&r.ax, &r.cx)
	case 0x92: // XCHG AX,DX
		xchg(&r.ax, &r.dx)
	case 0x93: // XCHG AX,BX
		xchg(&r.ax, &r.bx)
	case 0x94: // XCHG AX,SP
		xchg(&r.ax, &r.sp)
	case 0x95: // XCHG AX,BP
		xchg(&r.ax, &r.bp)
	case 0x96: // XCHG AX,SI
		xchg(&r.ax, &r.si)
	case 0x97: // XCHG AX,DI
		xchg(&r.ax, &r.di)
	default:
		log.Panic("invalid operation: ", op)
	}
}

func (r *Registers) AL() byte {
	v := byte(r.ax & 0xFF)
	validator.ReadReg8(validator.AL, v)
	return v
}

func (r *Registers) AH() byte {
	v := byte(r.ax >> 8)
	validator.ReadReg8(validator.AH, v)
	return v
}

func (r *Registers) AX() uint16 {
	validator.ReadReg16(validator.AX, r.ax)
	return r.ax
}

func (r *Registers) SetAL(v byte) {
	validator.WriteReg8(validator.AL, v)
	r.ax = r.ax&0xFF00 | uint16(v)
}

func (r *Registers) SetAH(v byte) {
	validator.WriteReg8(validator.AH, v)
	r.ax = r.ax&0xFF | uint16(v)<<8
}

func (r *Registers) SetAX(v uint16) {
	validator.WriteReg16(validator.AX, v)
	r.ax = v
}

func (r *Registers) BL() byte {
	v := byte(r.bx & 0xFF)
	validator.ReadReg8(validator.BL, v)
	return v
}

func (r *Registers) BH() byte {
	v := byte(r.bx >> 8)
	validator.ReadReg8(validator.BH, v)
	return v
}

func (r *Registers) BX() uint16 {
	validator.ReadReg16(validator.BX, r.bx)
	return r.bx
}

func (r *Registers) SetBL(v byte) {
	validator.WriteReg8(validator.BL, v)
	r.bx = r.bx&0xFF00 | uint16(v)
}

func (r *Registers) SetBH(v byte) {
	validator.WriteReg8(validator.BH, v)
	r.bx = r.bx&0xFF | uint16(v)<<8
}

func (r *Registers) SetBX(v uint16) {
	validator.WriteReg16(validator.BX, v)
	r.bx = v
}

func (r *Registers) CL() byte {
	v := byte(r.cx & 0xFF)
	validator.ReadReg8(validator.CL, v)
	return v
}

func (r *Registers) CH() byte {
	v := byte(r.cx >> 8)
	validator.ReadReg8(validator.CH, v)
	return v
}

func (r *Registers) CX() uint16 {
	validator.ReadReg16(validator.CX, r.cx)
	return r.cx
}

func (r *Registers) SetCL(v byte) {
	validator.WriteReg8(validator.CL, v)
	r.cx = r.cx&0xFF00 | uint16(v)
}

func (r *Registers) SetCH(v byte) {
	validator.WriteReg8(validator.CH, v)
	r.cx = r.cx&0xFF | uint16(v)<<8
}

func (r *Registers) SetCX(v uint16) {
	validator.WriteReg16(validator.CX, v)
	r.cx = v
}

func (r *Registers) DL() byte {
	v := byte(r.dx & 0xFF)
	validator.ReadReg8(validator.DL, v)
	return v
}

func (r *Registers) DH() byte {
	v := byte(r.dx >> 8)
	validator.ReadReg8(validator.DH, v)
	return v
}

func (r *Registers) DX() uint16 {
	validator.ReadReg16(validator.DX, r.dx)
	return r.dx
}

func (r *Registers) SetDL(v byte) {
	validator.WriteReg8(validator.DL, v)
	r.dx = r.dx&0xFF00 | uint16(v)
}

func (r *Registers) SetDH(v byte) {
	validator.WriteReg8(validator.DH, v)
	r.dx = r.dx&0xFF | uint16(v)<<8
}

func (r *Registers) SetDX(v uint16) {
	validator.WriteReg16(validator.DX, v)
	r.dx = v
}

func (r *Registers) SP() uint16 {
	validator.ReadReg16(validator.SP, r.sp)
	return r.sp
}

func (r *Registers) SetSP(v uint16) {
	validator.WriteReg16(validator.SP, v)
	r.sp = v
}

func (r *Registers) BP() uint16 {
	validator.ReadReg16(validator.BP, r.bp)
	return r.bp
}

func (r *Registers) SetBP(v uint16) {
	validator.WriteReg16(validator.BP, v)
	r.bp = v
}

func (r *Registers) SI() uint16 {
	validator.ReadReg16(validator.SI, r.si)
	return r.si
}

func (r *Registers) SetSI(v uint16) {
	validator.WriteReg16(validator.SI, v)
	r.si = v
}

func (r *Registers) DI() uint16 {
	validator.ReadReg16(validator.DI, r.di)
	return r.di
}

func (r *Registers) SetDI(v uint16) {
	validator.WriteReg16(validator.DI, v)
	r.di = v
}

func (r *Registers) ES() uint16 {
	validator.ReadReg16(validator.ES, r.es)
	return r.es
}

func (r *Registers) SetES(v uint16) {
	validator.WriteReg16(validator.ES, v)
	r.es = v
}

func (r *Registers) CS() uint16 {
	validator.ReadReg16(validator.CS, r.cs)
	return r.cs
}

func (r *Registers) SetCS(v uint16) {
	validator.WriteReg16(validator.CS, v)
	r.cs = v
}

func (r *Registers) SS() uint16 {
	validator.ReadReg16(validator.SS, r.ss)
	return r.ss
}

func (r *Registers) SetSS(v uint16) {
	validator.WriteReg16(validator.SS, v)
	r.ss = v
}

func (r *Registers) DS() uint16 {
	validator.ReadReg16(validator.DS, r.ds)
	return r.ds
}

func (r *Registers) SetDS(v uint16) {
	validator.WriteReg16(validator.DS, v)
	r.ds = v
}
