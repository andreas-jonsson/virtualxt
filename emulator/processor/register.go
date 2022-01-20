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
)

const (
	Carry           Flags = 0x001
	Parity          Flags = 0x004
	Adjust          Flags = 0x010
	Zero            Flags = 0x040
	Sign            Flags = 0x080
	Trap            Flags = 0x100
	InterruptEnable Flags = 0x200
	Direction       Flags = 0x400
	Overflow        Flags = 0x800
)

const AllFlags = Carry | Parity | Adjust | Zero | Sign | Trap | InterruptEnable | Direction | Overflow

type Flags uint16

func (r *Flags) Get(f Flags) Flags {
	return *r & f
}

func (r *Flags) GetBool(f Flags) bool {
	return r.Get(f) != 0
}

func (r *Flags) Set(f Flags) {
	*r |= f
}

func (r *Flags) SetBool(f Flags, b bool) {
	if b {
		r.Set(f)
		return
	}
	r.Clear(f)
}

func (r *Flags) Clear(f Flags) {
	*r &= ^f
}

func (r *Flags) Store(f uint16) {
	*r = (Flags(f) & AllFlags) | 0x2
}

func (r *Flags) Load() uint16 {
	return uint16((*r & AllFlags) | 0x2)
}

type Registers struct {
	ax, cx, dx, bx,
	sp, bp, si, di,
	es, cs, ss, ds uint16

	Flags

	IP    uint16
	Debug bool
}

func (r *Registers) Reset() {
	*r = Registers{}
	r.Flags.Store(0)
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
	return v
}

func (r *Registers) AH() byte {
	v := byte(r.ax >> 8)
	return v
}

func (r *Registers) AX() uint16 {
	return r.ax
}

func (r *Registers) SetAL(v byte) {
	r.ax = r.ax&0xFF00 | uint16(v)
}

func (r *Registers) SetAH(v byte) {
	r.ax = r.ax&0xFF | uint16(v)<<8
}

func (r *Registers) SetAX(v uint16) {
	r.ax = v
}

func (r *Registers) BL() byte {
	v := byte(r.bx & 0xFF)
	return v
}

func (r *Registers) BH() byte {
	v := byte(r.bx >> 8)
	return v
}

func (r *Registers) BX() uint16 {
	return r.bx
}

func (r *Registers) SetBL(v byte) {
	r.bx = r.bx&0xFF00 | uint16(v)
}

func (r *Registers) SetBH(v byte) {
	r.bx = r.bx&0xFF | uint16(v)<<8
}

func (r *Registers) SetBX(v uint16) {
	r.bx = v
}

func (r *Registers) CL() byte {
	v := byte(r.cx & 0xFF)
	return v
}

func (r *Registers) CH() byte {
	v := byte(r.cx >> 8)
	return v
}

func (r *Registers) CX() uint16 {
	return r.cx
}

func (r *Registers) SetCL(v byte) {
	r.cx = r.cx&0xFF00 | uint16(v)
}

func (r *Registers) SetCH(v byte) {
	r.cx = r.cx&0xFF | uint16(v)<<8
}

func (r *Registers) SetCX(v uint16) {
	r.cx = v
}

func (r *Registers) DL() byte {
	v := byte(r.dx & 0xFF)
	return v
}

func (r *Registers) DH() byte {
	v := byte(r.dx >> 8)
	return v
}

func (r *Registers) DX() uint16 {
	return r.dx
}

func (r *Registers) SetDL(v byte) {
	r.dx = r.dx&0xFF00 | uint16(v)
}

func (r *Registers) SetDH(v byte) {
	r.dx = r.dx&0xFF | uint16(v)<<8
}

func (r *Registers) SetDX(v uint16) {
	r.dx = v
}

func (r *Registers) SP() uint16 {
	return r.sp
}

func (r *Registers) SetSP(v uint16) {
	r.sp = v
}

func (r *Registers) BP() uint16 {
	return r.bp
}

func (r *Registers) SetBP(v uint16) {
	r.bp = v
}

func (r *Registers) SI() uint16 {
	return r.si
}

func (r *Registers) SetSI(v uint16) {
	r.si = v
}

func (r *Registers) DI() uint16 {
	return r.di
}

func (r *Registers) SetDI(v uint16) {
	r.di = v
}

func (r *Registers) ES() uint16 {
	return r.es
}

func (r *Registers) SetES(v uint16) {
	r.es = v
}

func (r *Registers) CS() uint16 {
	return r.cs
}

func (r *Registers) SetCS(v uint16) {
	r.cs = v
}

func (r *Registers) SS() uint16 {
	return r.ss
}

func (r *Registers) SetSS(v uint16) {
	r.ss = v
}

func (r *Registers) DS() uint16 {
	return r.ds
}

func (r *Registers) SetDS(v uint16) {
	r.ds = v
}

func (r *Registers) GetValues() [12]uint16 {
	return [12]uint16{
		r.AX(), r.CX(), r.DX(), r.BX(), 
		r.SP(), r.BP(), r.SI(), r.DI(), 
		r.ES(), r.CS(), r.SS(), r.DS(),
	}
}
