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

func (p *CPU) rotSHR8(v byte) byte {
	p.CF = v&1 != 0
	return v >> 1
}

func (p *CPU) rotSAR8(v byte) byte {
	s := v & 0x80
	p.CF = v&1 != 0
	return v>>1 | s
}

func (p *CPU) rotSHL8(v byte) byte {
	p.CF = v&0x80 != 0
	return v << 1
}

func (p *CPU) rotROL8(v byte) byte {
	s := v & 0x80
	p.CF = s != 0
	return v<<1 | s>>7
}

func (p *CPU) rotROR8(v byte) byte {
	c := v & 1
	p.CF = c != 0
	return v>>1 | c<<7
}

func (p *CPU) rotRCL8(v byte) byte {
	s := v & 0x80
	v <<= 1
	if p.CF {
		v |= 1
	}
	p.CF = s != 0
	return v
}

func (p *CPU) rotRCR8(v byte) byte {
	c := v & 1
	v >>= 1
	if p.CF {
		v |= 0x80
	}
	p.CF = c != 0
	return v
}

func (p *CPU) rotSHR16(v uint16) uint16 {
	p.CF = v&1 != 0
	return v >> 1
}

func (p *CPU) rotSAR16(v uint16) uint16 {
	s := v & 0x8000
	p.CF = v&1 != 0
	return v>>1 | s
}

func (p *CPU) rotSHL16(v uint16) uint16 {
	p.CF = v&0x8000 != 0
	return v << 1
}

func (p *CPU) rotROL16(v uint16) uint16 {
	s := v & 0x8000
	p.CF = s != 0
	return v<<1 | s>>15
}

func (p *CPU) rotROR16(v uint16) uint16 {
	c := v & 1
	p.CF = c != 0
	return v>>1 | c<<15
}

func (p *CPU) rotRCL16(v uint16) uint16 {
	s := v & 0x8000
	v <<= 1
	if p.CF {
		v |= 1
	}
	p.CF = s != 0
	return v
}

func (p *CPU) rotRCR16(v uint16) uint16 {
	c := v & 1
	v >>= 1
	if p.CF {
		v |= 0x8000
	}
	p.CF = c != 0
	return v
}

func (p *CPU) rotateOverflowLeft(num, sig byte) {
	p.OF = b2ui32(p.CF)^uint32(sig) != 0
}

func (p *CPU) rotateOverflowRight(num, sig2 byte) {
	p.OF = (sig2>>1)^(sig2&1) != 0
}

func (p *CPU) shiftOrRotate8(op, a, b byte) byte {
	// The OF flag is defined only for the 1-bit rotates; it is undefined in all other cases
	// (except that a zero-bit rotate does nothing, that is affects no flags). For left rotates,
	// the OF flag is set to the exclusive OR of the CF bit (after the rotate) and the
	// most-significant bit of the result. For right rotates, the OF flag is set to the
	// exclusive OR of the two most-significant bits of the result.

	if b == 0 {
		return a
	}

	if p.isV20 {
		b &= 0x1F
	}

	org := a

	for i := 0; i < int(b); i++ {
		switch op {
		case 0:
			a = p.rotROL8(a)
		case 1:
			a = p.rotROR8(a)
		case 2:
			a = p.rotRCL8(a)
		case 3:
			a = p.rotRCR8(a)
		case 4:
			a = p.rotSHL8(a)
		case 5:
			a = p.rotSHR8(a)
		case 7:
			a = p.rotSAR8(a)
		default:
			p.invalidOpcode()
		}
	}

	switch op {
	case 0:
		p.rotateOverflowLeft(b, byte(a>>7))
	case 1:
		p.rotateOverflowRight(b, byte(a>>6))
	case 2:
		p.rotateOverflowLeft(b, byte(a>>7))
	case 3:
		p.rotateOverflowRight(b, byte(a>>6))
	case 4:
		p.OF = !(byte(b2ui16(p.CF)) == (a >> 7))
		p.updateFlagsSZP8(a)
	case 5:
		p.OF = (b == 1) && (org&0x80 != 0)
		p.updateFlagsSZP8(a)
	case 7:
		p.OF = false
		p.updateFlagsSZP8(a)
	}
	return a
}

func (p *CPU) shiftOrRotate16(op byte, a uint16, b byte) uint16 {
	if b == 0 {
		return a
	}

	if p.isV20 {
		b &= 0x1F
	}
	org := a

	for i := 0; i < int(b); i++ {
		switch op {
		case 0:
			a = p.rotROL16(a)
		case 1:
			a = p.rotROR16(a)
		case 2:
			a = p.rotRCL16(a)
		case 3:
			a = p.rotRCR16(a)
		case 4:
			a = p.rotSHL16(a)
		case 5:
			a = p.rotSHR16(a)
		case 7:
			a = p.rotSAR16(a)
		default:
			p.invalidOpcode()
		}
	}

	switch op {
	case 0:
		p.rotateOverflowLeft(b, byte(a>>15))
	case 1:
		p.rotateOverflowRight(b, byte(a>>14))
	case 2:
		p.rotateOverflowLeft(b, byte(a>>15))
	case 3:
		p.rotateOverflowRight(b, byte(a>>14))
	case 4:
		p.OF = !(b2ui16(p.CF) == (a >> 15))
		p.updateFlagsSZP16(a)
	case 5:
		p.OF = (b == 1) && (org&0x8000 != 0)
		p.updateFlagsSZP16(a)
	case 7:
		p.OF = false
		p.updateFlagsSZP16(a)
	}
	return a
}
