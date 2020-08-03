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

package cpu

import (
	"log"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

var zero16 uint16

type instructionState struct {
	opcode, modRegRM,
	repeatMode byte

	isWide, rmToReg bool
	decodeAt        uint16
	segOverride     *uint16
	cycleCount      int
}

func (p *CPU) getReg() byte {
	return (p.modRegRM >> 3) & 7
}

func (p *CPU) regLocation() dataLocation {
	return dataLocation(p.getReg()) | registerLocation
}

func (p *CPU) segLocation() dataLocation {
	return dataLocation(p.getReg()) | segmentLocation
}

func (p *CPU) rmLocation() dataLocation {
	idx := p.modRegRM & 0xC7
	f := modRMLookup[idx]
	return f(p)
}

func (p *CPU) peakOpcodeStream() byte {
	return p.ReadByte(memory.NewPointer(p.CS, p.IP))
}

func (p *CPU) readOpcodeStream() byte {
	v := p.peakOpcodeStream()
	p.IP++
	return v
}

func (p *CPU) readOpcodeImm16() uint16 {
	v := p.ReadWord(memory.NewPointer(p.CS, p.IP))
	p.IP += 2
	return v
}

func (p *CPU) readModRegRM() {
	p.modRegRM = p.readOpcodeStream()
}

func (p *CPU) parseOperands() (dataLocation, dataLocation) {
	p.readModRegRM()
	reg, rm := p.regLocation(), p.rmLocation()
	if p.rmToReg {
		return reg, rm
	}
	return rm, reg
}

func (p *CPU) parseOpcode() {
	p.segOverride = nil
	p.repeatMode = 0
	p.decodeAt = p.IP

	var op byte
loop:
	for {
		switch op = p.readOpcodeStream(); op {
		case 0x26: // ES:
			p.segOverride = &p.ES
		case 0x2E: // CS:
			p.segOverride = &p.CS
		case 0x36: // SS:
			p.segOverride = &p.SS
		case 0x3E: // DS:
			p.segOverride = &p.DS
		case 0xF0: // LOCK
			// NOP...
		case 0xF1:
			//log.Printf("Unsupported instruction prefix: 0x%X", op)
			//p.invalidOpcode()
			//break loop
		case 0xF2, 0xF3: // REPNE/REPNZ,REP/REPE/REPZ
			p.repeatMode = op
		default:
			break loop
		}
	}

	p.opcode = op
	p.isWide = op&1 != 0
	p.rmToReg = op&2 != 0
}

func (p *CPU) packFlags8() byte {
	var flags byte = 0x2
	if p.CF {
		flags |= 0x001
	}
	if p.PF {
		flags |= 0x004
	}
	if p.AF {
		flags |= 0x010
	}
	if p.ZF {
		flags |= 0x040
	}
	if p.SF {
		flags |= 0x080
	}
	return flags
}

func (p *CPU) packFlags16() uint16 {
	flags := uint16(p.packFlags8())
	if p.TF {
		flags |= 0x100
	}
	if p.IF {
		flags |= 0x200
	}
	if p.DF {
		flags |= 0x400
	}
	if p.OF {
		flags |= 0x800
	}
	return flags
}

func (p *CPU) unpackFlags8(flags byte) {
	p.CF = flags&0x001 != 0
	p.PF = flags&0x004 != 0
	p.AF = flags&0x010 != 0
	p.ZF = flags&0x040 != 0
	p.SF = flags&0x080 != 0
}

func (p *CPU) unpackFlags16(flags uint16) {
	p.unpackFlags8(byte(flags & 0xFF))
	p.TF = flags&0x100 != 0
	p.IF = flags&0x200 != 0
	p.DF = flags&0x400 != 0
	p.OF = flags&0x800 != 0
}

func signExtend16(v byte) uint16 {
	if v&0x80 != 0 {
		return uint16(v) | 0xFF00
	}
	return uint16(v)
}

func signExtend32(v uint16) uint32 {
	if v&0x8000 != 0 {
		return uint32(v) | 0xFFFF0000
	}
	return uint32(v)
}

func (p *CPU) stackTop() memory.Pointer {
	return memory.NewPointer(p.SS, p.SP)
}

func (p *CPU) push16(v uint16) {
	p.SP -= 2
	p.WriteWord(p.stackTop(), v)
}

func (p *CPU) pop16() uint16 {
	v := p.ReadWord(p.stackTop())
	p.SP += 2
	return v
}

func (p *CPU) updateFlagsSZP8(res byte) {
	p.SF = res&0x80 != 0
	p.ZF = res == 0
	p.PF = parityLookup[res]
}

func (p *CPU) updateFlagsSZP16(res uint16) {
	p.SF = res&0x8000 != 0
	p.ZF = res == 0
	p.PF = parityLookup[res&0xFF]
}

func (p *CPU) getFlagsMask() (uint32, uint32) {
	maskC := uint32(0xFF00)
	maskO := uint32(0x0080)
	if p.isWide {
		maskC = 0xFFFF0000
		maskO = 0x00008000
	}
	return maskC, maskO
}

func (p *CPU) clearFlagsOC() {
	p.CF = false
	p.OF = false
}

func (p *CPU) updateFlagsOSZPCLog8(res byte) {
	p.updateFlagsSZP8(res)
	p.clearFlagsOC()
}

func (p *CPU) updateFlagsOSZPCLog16(res uint16) {
	p.updateFlagsSZP16(res)
	p.clearFlagsOC()
}

func (p *CPU) updateFlagsOACAdd(res, a, b uint32) {
	maskC, maskO := p.getFlagsMask()
	p.CF = res&maskC != 0
	p.AF = ((a ^ b ^ res) & 0x10) == 0x10
	p.OF = ((res ^ a) & (res ^ b) & maskO) == maskO
}

func (p *CPU) updateFlagsOACSub(res, a, b uint32) {
	maskC, maskO := p.getFlagsMask()
	p.CF = res&maskC != 0
	p.AF = ((a ^ b ^ res) & 0x10) != 0
	p.OF = ((res ^ a) & (a ^ b) & maskO) != 0
}

func (p *CPU) updateFlagsOACSubCarry(res, a, b uint32) {
	b += b2ui32(p.CF)
	p.updateFlagsOACSub(res, a, b)
}

func b2ui16(b bool) uint16 {
	if b {
		return 1
	}
	return 0
}

func b2ui32(b bool) uint32 {
	return uint32(b2ui16(b))
}

func opXCHG(a, b *uint16) {
	tmp := *a
	*a = *b
	*b = tmp
}

func (p *CPU) getSeg(seg uint16) uint16 {
	if p.segOverride != nil {
		return *p.segOverride
	}
	return seg
}

func (p *CPU) divisionByZero() {
	p.IP = p.decodeAt
	p.doInterrupt(0)
}

func (p *CPU) doInterrupt(n int) {
	p.stats.NumInterrupts++

	if handler := p.interceptors[n]; handler != nil {
		if err := handler.HandleInterrupt(n); err == nil {
			p.TF, p.IF = false, false
			return
		} else if err != processor.ErrInterruptNotHandled {
			log.Panic(err)
		}
	}

	p.push16(p.packFlags16())
	p.push16(p.CS)
	p.push16(p.IP)

	offset := n * 4
	p.CS = p.ReadWord(memory.Pointer(offset + 2))
	p.IP = p.ReadWord(memory.Pointer(offset))
	p.TF, p.IF = false, false
}

func (p *CPU) Step() (int, error) {
	// Reset cycle counter.
	p.cycleCount = 0

	if p.trap {
		p.doInterrupt(1)
	}
	p.trap = p.TF

	if !p.trap && p.IF {
		if n, err := p.pic.GetInterrupt(); err == nil {
			p.doInterrupt(n)
		}
	}

	p.parseOpcode()
	if p.repeatMode != 0 {
		err := p.doRepeat()
		return p.cycleCount, err
	}

	if err := p.execute(); err != nil {
		return p.cycleCount, err
	}

	for _, d := range p.peripherals {
		if err := d.Step(p.cycleCount); err != nil {
			return p.cycleCount, err
		}
	}
	return p.cycleCount, nil
}

func (p *CPU) execute() error {
	// All instructions are 1 cycle right now.
	p.cycleCount++

	op := p.opcode
	carry := op > 0x0F && p.CF
	carryOp := b2ui32(carry)

	switch op {
	case 0x00, 0x02, 0x10, 0x12: // ADD/ADC r/m8,r8
		dest, src := p.parseOperands()
		a, b := uint32(dest.readByte(p)), uint32(src.readByte(p))
		res := a + b + carryOp
		dest.writeByte(p, byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACAdd(res, a, b)
	case 0x01, 0x03, 0x11, 0x13: // ADD/ADC r/m16,r16
		dest, src := p.parseOperands()
		a, b := uint32(dest.readWord(p)), uint32(src.readWord(p))
		res := a + b + carryOp
		dest.writeWord(p, uint16(res))
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACAdd(res, a, b)
	case 0x04, 0x14: // ADD/ADC AL,d8
		a, b := uint32(p.AL()), uint32(p.readOpcodeStream())
		res := a + b + carryOp
		p.SetAL(byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACAdd(res, a, b)
	case 0x05, 0x15: // ADD/ADC AX,d16
		a, b := uint32(p.AX), uint32(p.readOpcodeImm16())
		res := a + b + carryOp
		p.AX = uint16(res)
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACAdd(res, a, b)
	case 0x06: // PUSH ES
		p.push16(p.ES)
	case 0x07: // POP ES
		p.ES = p.pop16()
	case 0x08, 0x0A: // OR r/m8,r8
		dest, src := p.parseOperands()
		a, b := dest.readByte(p), src.readByte(p)
		res := a | b
		dest.writeByte(p, res)
		p.updateFlagsOSZPCLog8(res)
	case 0x09, 0x0B: // OR r/m16,r16
		dest, src := p.parseOperands()
		a, b := dest.readWord(p), src.readWord(p)
		res := a | b
		dest.writeWord(p, res)
		p.updateFlagsOSZPCLog16(res)
	case 0x0C: // OR AL,d8
		a, b := p.AL(), p.readOpcodeStream()
		res := a | b
		p.SetAL(res)
		p.updateFlagsOSZPCLog8(res)
	case 0x0D: // OR AX,d16
		p.AX = p.AX | p.readOpcodeImm16()
		p.updateFlagsOSZPCLog16(p.AX)
	case 0x0E: // PUSH CS
		p.push16(p.CS)
	case 0x0F: // *POP CS
		if !p.isV20 {
			p.CS = p.pop16()
		}

	// 0x1x

	case 0x16: // PUSH SS
		p.push16(p.SS)
	case 0x17: // POP SS
		p.SS = p.pop16()
	case 0x18, 0x1A: // SBB r/m8,r8
		dest, src := p.parseOperands()
		a, b := uint32(dest.readByte(p)), uint32(src.readByte(p))
		res := a - (b + carryOp)
		dest.writeByte(p, byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACSubCarry(res, a, b)
	case 0x19, 0x1B: // SBB r/m16,r16
		dest, src := p.parseOperands()
		a, b := uint32(dest.readWord(p)), uint32(src.readWord(p))
		res := a - (b + carryOp)
		dest.writeWord(p, uint16(res))
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACSubCarry(res, a, b)
	case 0x1C: // SBB AL,d8
		a, b := uint32(p.AL()), uint32(p.readOpcodeStream())
		res := a - (b + carryOp)
		p.SetAL(byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACSubCarry(res, a, b)
	case 0x1D: // SBB AX,d16
		a, b := uint32(p.AX), uint32(p.readOpcodeImm16())
		res := a - (b + carryOp)
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACSubCarry(res, a, b)
	case 0x1E: // PUSH DS
		p.push16(p.DS)
	case 0x1F: // POP DS
		p.DS = p.pop16()

	// 0x2x

	case 0x20, 0x22: // AND r/m8,r8
		dest, src := p.parseOperands()
		a, b := dest.readByte(p), src.readByte(p)
		res := uint16(a) & uint16(b)
		dest.writeByte(p, byte(res))
		p.updateFlagsOSZPCLog8(byte(res))
	case 0x21, 0x23: // AND r/m16,r16
		dest, src := p.parseOperands()
		a, b := dest.readWord(p), src.readWord(p)
		res := uint32(a) & uint32(b)
		dest.writeWord(p, uint16(res))
		p.updateFlagsOSZPCLog16(uint16(res))
	case 0x24: // AND AL,d8
		a, b := p.AL(), p.readOpcodeStream()
		res := uint16(a) & uint16(b)
		p.SetAL(byte(res))
		p.updateFlagsOSZPCLog8(byte(res))
	case 0x25: // AND AX,d16
		b := p.readOpcodeImm16()
		res := uint32(p.AX) & uint32(b)
		p.AX = uint16(res)
		p.updateFlagsOSZPCLog16(uint16(res))
	case 0x27: // DAA
		if al := p.AL(); ((al & 0xF) > 9) || p.AF {
			v := uint16(al) + 6
			p.SetAL(byte(v & 0xFF))
			p.CF = (v & 0xFF00) != 0
			p.AF = true
		} else {
			p.AF = false
		}

		if al := p.AL(); ((al & 0xF0) > 0x90) || p.CF {
			p.SetAL(al + 0x60)
			p.CF = true
		} else {
			p.CF = false
		}

		al := p.AL()
		p.SetAL(al)
		p.updateFlagsSZP8(al)
	case 0x28, 0x2A: // SUB r/m8,r8
		dest, src := p.parseOperands()
		a, b := uint32(dest.readByte(p)), uint32(src.readByte(p))
		res := a - b
		dest.writeByte(p, byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACSub(res, a, b)
	case 0x29, 0x2B: // SUB r/m16,r16
		dest, src := p.parseOperands()
		a, b := uint32(dest.readWord(p)), uint32(src.readWord(p))
		res := a - b
		dest.writeWord(p, uint16(res))
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACSub(res, a, b)
	case 0x2C: // SUB AL,d8
		a, b := uint32(p.AL()), uint32(p.readOpcodeStream())
		res := a - b
		p.SetAL(byte(res))
		p.updateFlagsSZP8(byte(res))
		p.updateFlagsOACSub(res, a, b)
	case 0x2D: // SUB AX,d16
		a, b := uint32(p.AX), uint32(p.readOpcodeImm16())
		res := a - b
		p.AX = uint16(res)
		p.updateFlagsSZP16(uint16(res))
		p.updateFlagsOACSub(res, a, b)
	case 0x2F: // DAS
		if al := p.AL(); ((al & 15) > 9) || p.AF {
			v := uint16(al) - 6
			p.SetAL(byte(v & 0xFF))
			p.CF = (v & 0xFF00) != 0
			p.AF = true
		} else {
			p.AF = false
		}

		if al := p.AL(); ((al & 0xF0) > 0x90) || p.CF {
			p.SetAL(al - 0x60)
			p.CF = true
		} else {
			p.CF = false
		}
		p.updateFlagsSZP8(p.AL())

	// 0x3x

	case 0x30, 0x32: // XOR r/m8,r8
		dest, src := p.parseOperands()
		a, b := dest.readByte(p), src.readByte(p)
		res := uint16(a) ^ uint16(b)
		dest.writeByte(p, byte(res))
		p.updateFlagsOSZPCLog8(byte(res))
	case 0x31, 0x33: // XOR r/m16,r16
		dest, src := p.parseOperands()
		a, b := dest.readWord(p), src.readWord(p)
		res := uint32(a) ^ uint32(b)
		dest.writeWord(p, uint16(res))
		p.updateFlagsOSZPCLog16(uint16(res))
	case 0x34: // XOR AL,d8
		a, b := p.AL(), p.readOpcodeStream()
		res := uint16(a) ^ uint16(b)
		p.SetAL(byte(res))
		p.updateFlagsOSZPCLog8(byte(res))
	case 0x35: // XOR AX,d16
		b := p.readOpcodeImm16()
		res := uint32(p.AX) ^ uint32(b)
		p.AX = uint16(res)
		p.updateFlagsOSZPCLog16(uint16(res))
	case 0x37: // AAA
		if al := p.AL(); ((al & 0xF) > 9) || p.AF {
			p.SetAL(al + 6)
			p.SetAH(p.AH() + 1)
			p.AF, p.CF = true, true
		} else {
			p.AF, p.CF = false, false
		}
		al := p.AL() & 0xF
		p.SetAL(al)
		p.updateFlagsSZP8(al)
	case 0x38, 0x3A: // CMP r/m8,r8
		dest, src := p.parseOperands()
		a, b := uint32(dest.readByte(p)), uint32(src.readByte(p))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
	case 0x39, 0x3B: // CMP r/m16,r16
		dest, src := p.parseOperands()
		a, b := uint32(dest.readWord(p)), uint32(src.readWord(p))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
	case 0x3C: // CMP AL,d8
		a, b := uint32(p.AL()), uint32(p.readOpcodeStream())
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
	case 0x3D: // CMP AX,d16
		a, b := uint32(p.AX), uint32(p.readOpcodeImm16())
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
	case 0x3F: // AAS
		if al := p.AL(); ((al & 0xF) > 9) || p.AF {
			p.SetAL(al - 6)
			p.SetAH(p.AH() - 1)
			p.AF, p.CF = true, true
		} else {
			p.AF, p.CF = false, false
		}
		al := p.AL() & 0xF
		p.SetAL(al)
		p.updateFlagsSZP8(al)

	// 0x4x

	case 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47: // INC AX/CX/DX/BX/SP/BP/SI/DI
		reg := dataLocation(op-0x40) | registerLocation
		a := reg.readWord(p)
		res := a + 1
		reg.writeWord(p, res)

		cf := p.CF
		p.updateFlagsOACAdd(uint32(res), uint32(a), 1)
		p.updateFlagsSZP16(res)
		p.CF = cf
	case 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F: // DEC AX/CX/DX/BX/SP/BP/SI/DI
		reg := dataLocation(op-0x40) | registerLocation
		a := reg.readWord(p)
		res := a - 1
		reg.writeWord(p, res)

		cf := p.CF
		p.updateFlagsOACSub(uint32(res), uint32(a), 1)
		p.updateFlagsSZP16(res)
		p.CF = cf

	// 0x5x

	case 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57: // PUSH AX/CX/DX/BX/SP/BP/SI/DI
		p.push16((dataLocation(op-0x50) | registerLocation).readWord(p))
	case 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F: // POP AX/CX/DX/BX/SP/BP/SI/DI
		(dataLocation(op-0x58) | registerLocation).writeWord(p, p.pop16())

	// 0x6x

	case 0x60: // PUSHA (80186)
		if p.isV20 {
			sp := p.SP
			p.push16(p.AX)
			p.push16(p.CX)
			p.push16(p.DX)
			p.push16(p.BX)
			p.push16(sp)
			p.push16(p.BP)
			p.push16(p.SI)
			p.push16(p.DI)
		} else {
			p.invalidOpcode()
		}
	case 0x61: // POPA (80186)
		if p.isV20 {
			p.DI = p.pop16()
			p.SI = p.pop16()
			p.BP = p.pop16()
			p.pop16()
			p.BX = p.pop16()
			p.DX = p.pop16()
			p.CX = p.pop16()
			p.AX = p.pop16()
		} else {
			p.invalidOpcode()
		}
	case 0x62: // BOUND (80186)
		if p.isV20 {
			p.readModRegRM()
			idx := signExtend32(p.regLocation().readWord(p))
			addr := p.rmLocation().getAddress()

			if idx < signExtend32(p.ReadWord(addr.Pointer())) || idx > signExtend32(p.ReadWord(addr.AddInt(2).Pointer())) {
				p.doInterrupt(5)
			}
		} else {
			p.invalidOpcode()
		}
	case 0x69, 0x6B: // IMUL r/m16,d8/d16 (80186)
		if p.isV20 {
			p.readModRegRM()
			dest := p.rmLocation()
			a := signExtend32(dest.readWord(p))

			var res uint32
			if op == 69 {
				res = a * signExtend32(signExtend16(p.readOpcodeStream()))
			} else {
				res = a * signExtend32(p.readOpcodeImm16())
			}
			res16 := uint16(res & 0xFFFF)
			upper := uint16(res >> 16)

			dest.writeWord(p, res16)
			p.updateFlagsSZP16(res16)

			if res16&0x8000 == 0x8000 {
				p.CF = upper != 0xFFFF
			} else {
				p.CF = upper != 0x0
			}
			p.OF = p.CF
		} else {
			p.invalidOpcode()
		}
	case 0x68, 0x6A, 0x6C, 0x6D, 0x6E, 0x6F:
		if p.isV20 {
			log.Printf("opcode not implemented: 0x%X", op)
			p.Break()
		} else {
			p.invalidOpcode()
		}

	// 0x7x

	case 0x70: // JO rel8
		p.jmpRel8Cond(p.OF)
	case 0x71: // JNO rel8
		p.jmpRel8Cond(!p.OF)
	case 0x72: // JB/JNAE rel8
		p.jmpRel8Cond(p.CF)
	case 0x73: // JNB/JAE rel8
		p.jmpRel8Cond(!p.CF)
	case 0x74: // JE/JZ rel8
		p.jmpRel8Cond(p.ZF)
	case 0x75: // JNE/JNZ rel8
		p.jmpRel8Cond(!p.ZF)
	case 0x76: // JBE/JNA rel8
		p.jmpRel8Cond(p.CF || p.ZF)
	case 0x77: // JNBE/JA rel8
		p.jmpRel8Cond(!p.CF && !p.ZF)
	case 0x78: // JS rel8
		p.jmpRel8Cond(p.SF)
	case 0x79: // JNS rel8
		p.jmpRel8Cond(!p.SF)
	case 0x7A: // JP/JPE rel8
		p.jmpRel8Cond(p.PF)
	case 0x7B: // JNP/JPO rel8
		p.jmpRel8Cond(!p.PF)
	case 0x7C: // JL/JNGE rel8
		p.jmpRel8Cond(p.SF != p.OF)
	case 0x7D: // JNL/JGE rel8
		p.jmpRel8Cond(p.SF == p.OF)
	case 0x7E: // JLE/JNG rel8
		p.jmpRel8Cond(p.SF != p.OF || p.ZF)
	case 0x7F: // JNLE/JG rel8
		p.jmpRel8Cond(!p.ZF && p.SF == p.OF)

	// 0x8x

	case 0x80, 0x82: // _ALU1 r/m8,d8
		p.grp1()
	case 0x81, 0x83: // _ALU1 r/m16,d16
		p.grp1w()
	case 0x84: // TEST r/m8,r8
		p.readModRegRM()
		a, b := p.rmLocation().readByte(p), p.regLocation().readByte(p)
		p.updateFlagsOSZPCLog8(a & b)
	case 0x85: // TEST r/m16,r16
		p.readModRegRM()
		a, b := p.rmLocation().readWord(p), p.regLocation().readWord(p)
		p.updateFlagsOSZPCLog16(a & b)
	case 0x86: // XCHG r8,r/m8
		p.readModRegRM()
		dst, src := p.regLocation(), p.rmLocation()
		d, s := dst.readByte(p), src.readByte(p)
		dst.writeByte(p, s)
		src.writeByte(p, d)
	case 0x87: // XCHG r16,r/m16
		p.readModRegRM()
		dst, src := p.regLocation(), p.rmLocation()
		d, s := dst.readWord(p), src.readWord(p)
		dst.writeWord(p, s)
		src.writeWord(p, d)
	case 0x88, 0x8A: // MOV r/m8,r8
		dest, src := p.parseOperands()
		dest.writeByte(p, src.readByte(p))
	case 0x89, 0x8B: // MOV r/m16,r16
		dest, src := p.parseOperands()
		dest.writeWord(p, src.readWord(p))
	case 0x8C: // _MOV r/m16,sr
		p.readModRegRM()
		p.rmLocation().writeWord(p, p.segLocation().readWord(p))
	case 0x8D: // LEA r16,r/m16
		p.readModRegRM()
		p.segOverride = &zero16
		addr := p.rmLocation().getPointer()
		p.regLocation().writeWord(p, uint16(addr))
	case 0x8E: // _MOV sr,r/m16
		p.readModRegRM()
		p.segLocation().writeWord(p, p.rmLocation().readWord(p))
	case 0x8F: // _POP r/m16
		p.readModRegRM()
		p.rmLocation().writeWord(p, p.pop16())

	// 0x9x

	case 0x90: // NOP
		p.stats.NOP++
	case 0x91: // XCHG AX,CX
		opXCHG(&p.AX, &p.CX)
	case 0x92: // XCHG AX,DX
		opXCHG(&p.AX, &p.DX)
	case 0x93: // XCHG AX,BX
		opXCHG(&p.AX, &p.BX)
	case 0x94: // XCHG AX,SP
		opXCHG(&p.AX, &p.SP)
	case 0x95: // XCHG AX,BP
		opXCHG(&p.AX, &p.BP)
	case 0x96: // XCHG AX,SI
		opXCHG(&p.AX, &p.SI)
	case 0x97: // XCHG AX,DI
		opXCHG(&p.AX, &p.DI)
	case 0x98: // CBW
		p.AX = signExtend16(p.AL())
	case 0x99: // CWD
		if p.AX&0x8000 != 0 {
			p.DX = 0xFFFF
		} else {
			p.DX = 0
		}
	case 0x9A: // CALL seg:a16
		ip := p.readOpcodeImm16()
		cs := p.readOpcodeImm16()
		p.push16(p.CS)
		p.push16(p.IP)
		p.IP, p.CS = ip, cs
	case 0x9B: // WAIT
	case 0x9C: // PUSHF
		p.push16(p.packFlags16())
	case 0x9D: // POPF
		p.unpackFlags16(p.pop16())
	case 0x9E: // SAHF
		p.unpackFlags8(p.AH())
	case 0x9F: // LAHF
		p.SetAH(p.packFlags8())

	// 0xAx

	case 0xA0: // MOV AL,[addr]
		p.SetAL(p.ReadByte(memory.NewPointer(p.getSeg(p.DS), p.readOpcodeImm16())))
	case 0xA1: // MOV AX,[addr]
		p.AX = p.ReadWord(memory.NewPointer(p.getSeg(p.DS), p.readOpcodeImm16()))
	case 0xA2: // MOV [addr],AL
		p.WriteByte(memory.NewPointer(p.getSeg(p.DS), p.readOpcodeImm16()), p.AL())
	case 0xA3: // MOV [addr],AX
		p.WriteWord(memory.NewPointer(p.getSeg(p.DS), p.readOpcodeImm16()), p.AX)
	case 0xA4: // MOVSB
		p.WriteByte(memory.NewPointer(p.ES, p.DI), p.ReadByte(memory.NewPointer(p.getSeg(p.DS), p.SI)))
		p.updateDISI()
	case 0xA5: // MOVSW
		p.WriteWord(memory.NewPointer(p.ES, p.DI), p.ReadWord(memory.NewPointer(p.getSeg(p.DS), p.SI)))
		p.updateDISI()
	case 0xA6: // CMPSB
		a, b := uint32(p.ReadByte(memory.NewPointer(p.getSeg(p.DS), p.SI))), uint32(p.ReadByte(memory.NewPointer(p.ES, p.DI)))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
		p.updateDISI()
	case 0xA7: // CMPSW
		a, b := uint32(p.ReadWord(memory.NewPointer(p.getSeg(p.DS), p.SI))), uint32(p.ReadWord(memory.NewPointer(p.ES, p.DI)))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
		p.updateDISI()
	case 0xA8: // TEST AL,d8
		p.updateFlagsSZP8(p.AL() & p.readOpcodeStream())
	case 0xA9: // TEST AX,d16
		p.updateFlagsSZP16(p.AX & p.readOpcodeImm16())
	case 0xAA: // STOSB
		p.WriteByte(memory.NewPointer(p.ES, p.DI), p.AL())
		p.updateDI()
	case 0xAB: // STOSW
		p.WriteWord(memory.NewPointer(p.ES, p.DI), p.AX)
		p.updateDI()
	case 0xAC: // LODSB
		p.SetAL(p.ReadByte(memory.NewPointer(p.getSeg(p.DS), p.SI)))
		p.updateSI()
	case 0xAD: // LODSW
		p.AX = p.ReadWord(memory.NewPointer(p.getSeg(p.DS), p.SI))
		p.updateSI()
	case 0xAE: // SCASB
		a, b := uint32(p.AL()), uint32(p.ReadByte(memory.NewPointer(p.ES, p.DI)))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
		p.updateDI()
	case 0xAF: // SCASW
		a, b := uint32(p.AX), uint32(p.ReadWord(memory.NewPointer(p.ES, p.DI)))
		res := a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
		p.updateDI()

	// 0xBx

	case 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7: // MOV AL/CL/DL/BL/AH/CH/DH/BH,d8
		(dataLocation(op-0xB0) | registerLocation).writeByte(p, p.readOpcodeStream())
	case 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF: // MOV AX/CX/DX/BX/SP/BP/SI/DI,d16
		(dataLocation(op-0xB8) | registerLocation).writeWord(p, p.readOpcodeImm16())

	// 0xCx

	case 0xC0: // SHL r/m8,d8 (80186)
		if p.isV20 {
			p.readModRegRM()
			dest := p.rmLocation()
			dest.writeByte(p, p.shiftOrRotate8(p.getReg(), dest.readByte(p), p.readOpcodeStream()))
		} else {
			p.invalidOpcode()
		}
	case 0xC1: // SHL r/m16,d16 (80186)
		if p.isV20 {
			p.readModRegRM()
			dest := p.rmLocation()
			dest.writeWord(p, p.shiftOrRotate16(p.getReg(), dest.readWord(p), p.readOpcodeStream()))
		} else {
			p.invalidOpcode()
		}
	case 0xC2: // RET d16
		ip := p.pop16()
		p.SP += p.readOpcodeImm16()
		p.IP = ip
	case 0xC3: // RET
		p.IP = p.pop16()
	case 0xC4: // LES r16,m32
		p.readModRegRM()
		addr := p.rmLocation().getAddress()
		p.regLocation().writeWord(p, p.ReadWord(addr.Pointer()))
		p.ES = p.ReadWord(addr.AddInt(2).Pointer())
	case 0xC5: // LDS r16,m32
		p.readModRegRM()
		addr := p.rmLocation().getAddress()
		p.regLocation().writeWord(p, p.ReadWord(addr.Pointer()))
		p.DS = p.ReadWord(addr.AddInt(2).Pointer())
	case 0xC6: // _MOV r/m8,d8
		p.readModRegRM()
		p.rmLocation().writeByte(p, p.readOpcodeStream())
	case 0xC7: // _MOV r/m16,d16
		p.readModRegRM()
		p.rmLocation().writeWord(p, p.readOpcodeImm16())
	case 0xC8: // ENTER (80186)
		p.invalidOpcode()
	case 0xC9: // LEAVE (80186)
		p.invalidOpcode()
	case 0xCA: // RETF d16
		sp := p.readOpcodeImm16()
		p.IP = p.pop16()
		p.CS = p.pop16()
		p.SP += sp
	case 0xCB: // RETF
		p.IP = p.pop16()
		p.CS = p.pop16()
	case 0xCC: // INT 3
		p.doInterrupt(3)
	case 0xCD: // INT d8
		p.doInterrupt(int(p.readOpcodeStream()))
	case 0xCE: // INTO
		if p.OF {
			p.doInterrupt(4)
		}
	case 0xCF: // IRET
		p.IP = p.pop16()
		p.CS = p.pop16()
		p.unpackFlags16(p.pop16())

	// 0xDx

	case 0xD0: // _ROT r/m8,1
		p.readModRegRM()
		dest := p.rmLocation()
		dest.writeByte(p, p.shiftOrRotate8(p.getReg(), dest.readByte(p), 1))
	case 0xD1: // _ROT r/m16,1
		p.readModRegRM()
		dest := p.rmLocation()
		dest.writeWord(p, p.shiftOrRotate16(p.getReg(), dest.readWord(p), 1))
	case 0xD2: // _ROT r/m8,CL
		p.readModRegRM()
		dest := p.rmLocation()
		dest.writeByte(p, p.shiftOrRotate8(p.getReg(), dest.readByte(p), p.CL()))
	case 0xD3: // _ROT r/m16,CL
		p.readModRegRM()
		dest := p.rmLocation()
		dest.writeWord(p, p.shiftOrRotate16(p.getReg(), dest.readWord(p), p.CL()))
	case 0xD4: // AAM *d8
		if a, b := p.AL(), p.readOpcodeStream(); b == 0 {
			p.divisionByZero()
		} else {
			p.SetAH(a / b)
			p.SetAL(a % b)
			p.updateFlagsSZP16(p.AX)
		}
	case 0xD5: // AAD *d8
		p.AX = (uint16(p.AL()) + uint16(p.AH())*uint16(p.readOpcodeStream())) & 0xFF
		p.updateFlagsSZP16(p.AX)
	case 0xD6: // *SALC
		// This is XLAT on V20.
		if !p.isV20 {
			if p.CF {
				p.SetAL(0xFF)
			} else {
				p.SetAL(0)
			}
			break
		}
		fallthrough
	case 0xD7: // XLAT
		p.SetAL(p.ReadByte(memory.NewPointer(p.getSeg(p.DS), p.BX+uint16(p.AL()))))
	case 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF: // ESC
		p.readModRegRM()
		_, src := p.parseOperands()
		src.readByte(p)

	// 0xEx

	case 0xE0: // LOOPNZ/NE rel8
		p.CX--
		p.jmpRel8Cond(p.CX != 0 && !p.ZF)
	case 0xE1: // LOOPZ/E rel8
		p.CX--
		p.jmpRel8Cond(p.CX != 0 && p.ZF)
	case 0xE2: // LOOP rel8
		p.CX--
		p.jmpRel8Cond(p.CX != 0)
	case 0xE3: // JCXZ rel8
		p.jmpRel8Cond(p.CX == 0)
	case 0xE4: // IN AL,[d8]
		p.SetAL(p.InByte(uint16(p.readOpcodeStream())))
	case 0xE5: // IN AX,[d8]
		p.AX = p.InWord(uint16(p.readOpcodeStream()))
	case 0xE6: // OUT [d8],AL
		p.OutByte(uint16(p.readOpcodeStream()), p.AL())
	case 0xE7: // OUT [d8],AX
		p.OutWord(uint16(p.readOpcodeStream()), p.AX)
	case 0xE8: // CALL rel16
		p.push16(p.jmpRel16())
	case 0xE9: // JMP rel16
		p.jmpRel16()
	case 0xEA: // JMP seg:a16
		ip := p.readOpcodeImm16()
		p.CS = p.readOpcodeImm16()
		p.IP = ip
	case 0xEB: // JMP rel8
		p.jmpRel8()
	case 0xEC: // IN AL,[DX]
		p.SetAL(p.InByte(p.DX))
	case 0xED: // IN AX,[DX]
		p.AX = p.InWord(p.DX)
	case 0xEE: // OUT [DX],AL
		p.OutByte(p.DX, p.AL())
	case 0xEF: // OUT [DX],AX
		p.OutWord(p.DX, p.AX)

	// 0xFx

	case 0xF4: // HLT
		p.IP--
		return processor.ErrCPUHalt
	case 0xF5: // CMC
		p.CF = !p.CF
	case 0xF6: // _ALU2 r/m8,d8
		p.grp3a()
	case 0xF7: // _ALU2 r/m16,d16
		p.grp3b()
	case 0xF8: // CLC
		p.CF = false
	case 0xF9: // STC
		p.CF = true
	case 0xFA: // CLI
		p.IF = false
	case 0xFB: // STI
		p.IF = true
	case 0xFC: // CLD
		p.DF = false
	case 0xFD: // STD
		p.DF = true
	case 0xFE: // _MISC r/m8
		p.grp4()
	case 0xFF: // _MISC r/m16
		p.grp5()
	default:
		p.invalidOpcode()
	}

	p.stats.NumInstructions++
	return nil
}

func (p *CPU) grp1() {
	p.readModRegRM()
	dest := p.rmLocation()
	a, b := uint32(dest.readByte(p)), uint32(p.readOpcodeStream())

	var res uint32
	switch op := p.getReg(); op {
	case 0:
		res = a + b
		p.updateFlagsOACAdd(res, a, b)
	case 1:
		res = a | b
		p.clearFlagsOC()
	case 2:
		res = a + b + b2ui32(p.CF)
		p.updateFlagsOACAdd(res, a, b)
	case 3:
		res = a - (b + b2ui32(p.CF))
		p.updateFlagsOACSubCarry(res, a, b)
	case 4:
		res = a & b
		p.clearFlagsOC()
	case 5:
		res = a - b
		p.updateFlagsOACSub(res, a, b)
	case 6:
		res = a ^ b
		p.clearFlagsOC()
	case 7:
		res = a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
		return
	default:
		log.Panicf("invalid opcode: grp1(0x%X)", op)
	}

	dest.writeByte(p, byte(res))
	p.updateFlagsSZP8(byte(res))
}

func (p *CPU) grp1w() {
	p.readModRegRM()
	dest := p.rmLocation()
	a := uint32(dest.readWord(p))

	// Not super nice. :(
	var b uint32
	if p.opcode == 0x83 {
		b = uint32(signExtend16(p.readOpcodeStream()))
	} else {
		b = uint32(p.readOpcodeImm16())
	}

	var res uint32
	switch op := p.getReg(); op {
	case 0:
		res = a + b
		p.updateFlagsOACAdd(res, a, b)
	case 1:
		res = a | b
		p.clearFlagsOC()
	case 2:
		res = a + b + b2ui32(p.CF)
		p.updateFlagsOACAdd(res, a, b)
	case 3:
		res = a - (b + b2ui32(p.CF))
		p.updateFlagsOACSubCarry(res, a, b)
	case 4:
		res = a & b
		p.clearFlagsOC()
	case 5:
		res = a - b
		p.updateFlagsOACSub(res, a, b)
	case 6:
		res = a ^ b
		p.clearFlagsOC()
	case 7:
		res = a - b
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
		return
	default:
		log.Panicf("invalid opcode: grp1w(0x%X)", op)
	}

	dest.writeWord(p, uint16(res))
	p.updateFlagsSZP16(uint16(res))
}

func (p *CPU) opDIV8(a uint16, b byte) {
	if b == 0 {
		p.divisionByZero()
		return
	}

	if res := a / uint16(b); res > 0xFF {
		p.divisionByZero()
	} else {
		p.SetAL(byte(res))
		p.SetAH(byte(a % uint16(b)))
	}
}

func (p *CPU) opIDIV8(a uint16, b byte) {
	// Reference: fake86's - cpu.c

	if b == 0 {
		p.divisionByZero()
		return
	}

	d := signExtend16(b)
	sign := ((a ^ d) & 0x8000) != 0

	if a >= 0x8000 {
		a = (^a + 1) & 0xFFFF
	}
	if d >= 0x8000 {
		d = (^d + 1) & 0xFFFF
	}

	res1, res2 := a/d, a%d
	if res1&0xFF00 != 0 {
		p.divisionByZero()
		return
	}

	if sign {
		res1 = (^res1 + 1) & 0xFF
		res2 = (^res2 + 1) & 0xFF
	}

	p.SetAL(byte(res1))
	p.SetAH(byte(res2))
}

func (p *CPU) opDIV16(a uint32, b uint16) {
	if b == 0 {
		p.divisionByZero()
		return
	}

	if res := a / uint32(b); res > 0xFFFF {
		p.divisionByZero()
	} else {
		p.AX, p.DX = uint16(res), uint16(a%uint32(b))
	}
}

func (p *CPU) opIDIV16(a uint32, b uint16) {
	// Reference: fake86's - cpu.c

	if b == 0 {
		p.divisionByZero()
		return
	}

	d := signExtend32(b)
	sign := ((a ^ d) & 0x80000000) != 0

	if a >= 0x80000000 {
		a = (^a + 1) & 0xFFFFFFFF
	}
	if d >= 0x80000000 {
		d = (^d + 1) & 0xFFFFFFFF
	}

	res1, res2 := a/d, a%d
	if res1&0xFFFF0000 != 0 {
		p.divisionByZero()
		return
	}

	if sign {
		res1 = (^res1 + 1) & 0xFFFF
		res2 = (^res2 + 1) & 0xFFFF
	}
	p.AX, p.DX = uint16(res1), uint16(res2)
}

func (p *CPU) grp3a() {
	p.readModRegRM()
	operand := p.rmLocation()

	switch op := p.getReg(); op {
	case 0, 1:
		a, b := operand.readByte(p), p.readOpcodeStream()
		p.updateFlagsSZP8(a & b)
		p.clearFlagsOC()
	case 2:
		operand.writeByte(p, ^operand.readByte(p))
	case 3:
		a, b := uint32(0), uint32(operand.readByte(p))
		res := a - b
		operand.writeByte(p, byte(res))
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP8(byte(res))
		p.CF = b != 0
	case 4:
		a, b := uint32(p.AL()), uint32(operand.readByte(p))
		res := a * b
		p.AX = uint16(res & 0xFFFF)
		p.updateFlagsSZP8(uint8(res))
		p.CF = p.AH() != 0
		p.OF = p.CF
		if !p.isV20 {
			p.ZF = false
		}
	case 5:
		p.AX = signExtend16(p.AL()) * signExtend16(operand.readByte(p))
		p.updateFlagsSZP8(byte(p.AX))
		if p.AL()&0x80 == 0x80 {
			p.CF = p.AH() != 0xFF
		} else {
			p.CF = p.AH() != 0x0
		}
		p.OF = p.CF
		if !p.isV20 {
			p.ZF = false
		}
	case 6:
		p.opDIV8(p.AX, operand.readByte(p))
	case 7:
		p.opIDIV8(p.AX, operand.readByte(p))
	default:
		log.Panicf("invalid opcode: grp3a(0x%X)", op)
	}
}

func (p *CPU) grp3b() {
	p.readModRegRM()
	operand := p.rmLocation()

	switch op := p.getReg(); op {
	case 0, 1:
		a, b := operand.readWord(p), p.readOpcodeImm16()
		p.updateFlagsSZP16(a & b)
		p.clearFlagsOC()
	case 2:
		operand.writeWord(p, ^operand.readWord(p))
	case 3:
		a, b := uint32(0), uint32(operand.readWord(p))
		res := a - b
		operand.writeWord(p, uint16(res))
		p.updateFlagsOACSub(res, a, b)
		p.updateFlagsSZP16(uint16(res))
		p.CF = b != 0
	case 4:
		a, b := uint32(p.AX), uint32(operand.readWord(p))
		res := a * b
		p.DX, p.AX = uint16(res>>16), uint16(res&0xFFFF)
		p.updateFlagsSZP16(uint16(res))
		p.CF = p.DX != 0
		p.OF = p.CF
		if !p.isV20 {
			p.ZF = false
		}
	case 5:
		res := signExtend32(p.AX) * signExtend32(operand.readWord(p))
		p.DX, p.AX = uint16(res>>16), uint16(res&0xFFFF)
		p.updateFlagsSZP16(uint16(res))
		if p.AX&0x8000 == 0x8000 {
			p.CF = p.DX != 0xFFFF
		} else {
			p.CF = p.DX != 0x0
		}
		p.OF = p.CF
		if !p.isV20 {
			p.ZF = false
		}
	case 6:
		p.opDIV16((uint32(p.DX)<<16)|uint32(p.AX), operand.readWord(p))
	case 7:
		p.opIDIV16((uint32(p.DX)<<16)|uint32(p.AX), operand.readWord(p))
	default:
		log.Panicf("invalid opcode: grp3b(0x%X)", op)
	}
}

func (p *CPU) grp4() {
	p.readModRegRM()
	dest := p.rmLocation()
	v := uint32(dest.readByte(p))
	cf := p.CF

	var res uint32
	switch op := p.getReg(); op {
	case 0:
		res = v + 1
		p.updateFlagsOACAdd(res, v, 1)
	case 1:
		res = v - 1
		p.updateFlagsOACSub(res, v, 1)
	default:
		p.invalidOpcode()
		return
	}

	dest.writeByte(p, byte(res))
	p.updateFlagsSZP8(byte(res))
	p.CF = cf
}

func (p *CPU) grp5() {
	p.readModRegRM()
	dest := p.rmLocation()
	v := uint32(dest.readWord(p))
	cf := p.CF

	var res uint32
	switch op := p.getReg(); op {
	case 0:
		res = v + 1
		p.updateFlagsOACAdd(res, v, 1)
	case 1:
		res = v - 1
		p.updateFlagsOACSub(res, v, 1)
	case 2:
		p.push16(p.IP)
		p.IP = uint16(v)
		return
	case 3:
		p.push16(p.CS)
		p.push16(p.IP)
		p.IP = uint16(v)
		p.CS = p.ReadWord(dest.getAddress().AddInt(2).Pointer())
		return
	case 4:
		p.IP = uint16(v)
		return
	case 5:
		p.IP = uint16(v)
		p.CS = p.ReadWord(dest.getAddress().AddInt(2).Pointer())
		return
	case 6:
		p.push16(uint16(v))
		return
	default:
		p.invalidOpcode()
		return
	}

	dest.writeWord(p, uint16(res))
	p.updateFlagsSZP16(uint16(res))
	p.CF = cf
}

func (p *CPU) jmpRel8() uint16 {
	diff := uint16(int8(p.readOpcodeStream()))
	ip := p.IP
	p.IP += diff
	return ip
}

func (p *CPU) jmpRel16() uint16 {
	diff := p.readOpcodeImm16()
	ip := p.IP
	p.IP += diff
	return ip
}

func (p *CPU) jmpRel8Cond(cond bool) {
	if cond {
		p.jmpRel8()
	} else {
		p.readOpcodeStream()
	}
}

func (p *CPU) invalidOpcode() {
	log.Printf("invalid opcode: 0x%X", p.opcode)
	if p.isV20 {
		p.IP = p.decodeAt
		p.doInterrupt(6)
	} else {
		p.Break()
	}
}

func (p *CPU) isValidRepeat() (bool, bool) {
	switch p.opcode {
	case 0x6C, 0x6D, 0x6E, 0x6F:
		if p.isV20 {
			return false, false
		}
		fallthrough
	case 0xA4, 0xA5, 0xAC, 0xAD, 0xAA, 0xAB:
		return true, false
	case 0xA6, 0xA7, 0xAE, 0xAF:
		return true, true
	}
	return false, false
}

func (p *CPU) doRepeat() error {
	if valid, primitive := p.isValidRepeat(); valid {
		ip := p.IP
		for p.CX > 0 {
			p.IP = ip
			if err := p.execute(); err != nil {
				return err
			}
			p.CX--

			if primitive && ((p.repeatMode == 0xF2 && p.ZF) || (p.repeatMode == 0xF3 && !p.ZF)) {
				break
			}
		}
	} else {
		p.repeatMode = 0
		if err := p.execute(); err != nil {
			return err
		}
	}
	return nil
}

func (p *CPU) updateDI() {
	var n uint16 = 1
	if p.isWide {
		n = 2
	}
	if p.DF {
		p.DI -= n
	} else {
		p.DI += n
	}
}

func (p *CPU) updateSI() {
	var n uint16 = 1
	if p.isWide {
		n = 2
	}
	if p.DF {
		p.SI -= n
	} else {
		p.SI += n
	}
}

func (p *CPU) updateDISI() {
	p.updateDI()
	p.updateSI()
}
