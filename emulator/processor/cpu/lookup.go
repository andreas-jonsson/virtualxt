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

package cpu

import (
	"github.com/andreas-jonsson/virtualxt/emulator/memory"
)

const (
	registerLocation = 1 << 63
	segmentLocation  = 1 << 62
)

type dataLocation uint64

func (addr dataLocation) getAddress() memory.Address {
	return memory.Address(addr & 0xFFFFFFFF)
}

func (addr dataLocation) getPointer() memory.Pointer {
	return addr.getAddress().Pointer()
}

func (addr dataLocation) readByte(p *CPU) byte {
	if addr&registerLocation != 0 {
		switch addr & 0x7 {
		case 0:
			return p.AL()
		case 1:
			return p.CL()
		case 2:
			return p.DL()
		case 3:
			return p.BL()
		case 4:
			return p.AH()
		case 5:
			return p.CH()
		case 6:
			return p.DH()
		case 7:
			return p.BH()
		}
	}
	return p.ReadByte(addr.getPointer())
}

func (addr dataLocation) writeByte(p *CPU, data byte) {
	if addr&registerLocation != 0 {
		switch addr & 0x7 {
		case 0:
			p.SetAL(data)
		case 1:
			p.SetCL(data)
		case 2:
			p.SetDL(data)
		case 3:
			p.SetBL(data)
		case 4:
			p.SetAH(data)
		case 5:
			p.SetCH(data)
		case 6:
			p.SetDH(data)
		case 7:
			p.SetBH(data)
		}
		return
	}
	p.WriteByte(addr.getPointer(), data)
}

func (addr dataLocation) readWord(p *CPU) uint16 {
	if addr&registerLocation != 0 {
		switch addr & 0x7 {
		case 0:
			return p.AX
		case 1:
			return p.CX
		case 2:
			return p.DX
		case 3:
			return p.BX
		case 4:
			return p.SP
		case 5:
			return p.BP
		case 6:
			return p.SI
		case 7:
			return p.DI
		}
	} else if addr&segmentLocation != 0 {
		switch addr & 0x7 {
		case 0:
			return p.ES
		case 1:
			return p.CS
		case 2:
			return p.SS
		case 3:
			return p.DS
		default:
			panic("invalid data location")
		}
	}
	return p.ReadWord(addr.getPointer())
}

func (addr dataLocation) writeWord(p *CPU, data uint16) {
	if addr&registerLocation != 0 {
		switch addr & 0x7 {
		case 0:
			p.AX = data
		case 1:
			p.CX = data
		case 2:
			p.DX = data
		case 3:
			p.BX = data
		case 4:
			p.SP = data
		case 5:
			p.BP = data
		case 6:
			p.SI = data
		case 7:
			p.DI = data
		}
		return
	} else if addr&segmentLocation != 0 {
		switch addr & 0x7 {
		case 0:
			p.ES = data
		case 1:
			p.CS = data
		case 2:
			p.SS = data
		case 3:
			p.DS = data
		default:
			panic("invalid data location")
		}
		return
	}
	p.WriteWord(addr.getPointer(), data)
}

var regFunc = func(p *CPU) dataLocation {
	return dataLocation((p.modRegRM&0xC7)-0xC0) | registerLocation
}

// Effective address calculation.
var modRMLookup = [208]func(*CPU) dataLocation{
	// 0x0x

	// DS:[BX+SI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.SI)) },

	// DS:[BX+DI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.DI)) },

	// SS:[BP+SI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.SI)) },

	// SS:[BP+DI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.DI)) },

	// DS:[SI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.SI)) },

	// DS:[DI]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.DI)) },

	// DS:[a16]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.readOpcodeImm16())) },

	// DS:[BX]
	func(p *CPU) dataLocation { return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX)) },

	nil, nil, nil, nil, nil, nil, nil, nil,

	// 0x1x

	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,

	// 0x4x

	// DS:[BX+SI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.SI+uint16(diff)))
	},

	// DS:[BX+DI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.DI+uint16(diff)))
	},

	// SS:[BP+SI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.SI+uint16(diff)))
	},

	// SS:[BP+DI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.DI+uint16(diff)))
	},

	// DS:[SI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.SI+uint16(diff)))
	},

	// DS:[DI+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.DI+uint16(diff)))
	},

	// SS:[BP+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+uint16(diff)))
	},

	// DS:[BX+rel8]
	func(p *CPU) dataLocation {
		diff := int8(p.readOpcodeStream())
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+uint16(diff)))
	},

	nil, nil, nil, nil, nil, nil, nil, nil,

	// 0x5x

	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,

	// 0x8x

	// DS:[BX+SI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.SI+p.readOpcodeImm16()))
	},

	// DS:[BX+DI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.DI+p.readOpcodeImm16()))
	},

	// SS:[BP+SI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.SI+p.readOpcodeImm16()))
	},

	// SS:[BP+DI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.DI+p.readOpcodeImm16()))
	},

	// DS:[SI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.SI+p.readOpcodeImm16()))
	},

	// DS:[DI+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.DI+p.readOpcodeImm16()))
	},

	// SS:[BP+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.SS), p.BP+p.readOpcodeImm16()))
	},

	// DS:[BX+rel16]
	func(p *CPU) dataLocation {
		return dataLocation(memory.NewAddress(p.getSeg(p.DS), p.BX+p.readOpcodeImm16()))
	},

	nil, nil, nil, nil, nil, nil, nil, nil,

	// 0x9x

	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
	nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,

	// 0xCx

	regFunc, regFunc, regFunc, regFunc, regFunc, regFunc, regFunc, regFunc,

	nil, nil, nil, nil, nil, nil, nil, nil,
}

var parityLookup = [256]bool{
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
	true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
}
