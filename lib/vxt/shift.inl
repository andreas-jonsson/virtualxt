/*
Copyright (c) 2019-2022 Andreas T Jonsson

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

#include "common.h"
#include "cpu.h"

// The OF flag is defined only for the 1-bit rotates; it is undefined in all other cases
// (except that a zero-bit rotate does nothing, that is affects no flags). For left rotates,
// the OF flag is set to the exclusive OR of the CF bit (after the rotate) and the
// most-significant bit of the result. For right rotates, the OF flag is set to the
// exclusive OR of the two most-significant bits of the result.

#define OVERFLOW_LEFT(sig) SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, (p->regs.flags & VXT_CARRY) ^ (vxt_word)(sig))
#define OVERFLOW_RIGHT(sig2) SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, ((sig2) >> 1) ^ ((sig2) & 1))

static vxt_byte bitshift_8(CONSTSP(cpu) p, vxt_byte v, vxt_byte c) {
	if (c == 0)
		return v;

   #ifdef VXT_CPU_286
	   c &= 0x1F;
   #endif

   vxt_byte o = v;
   vxt_byte s;

	for (int i = 0; i < c; i++) {
		switch (p->mode.reg) {
         case 0: // ROL r/m8
            s = v & 0x80;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            v = (v << 1) | (s >> 7);
            break;
         case 1: // ROR r/m8
            s = v & 1;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            v = (v >> 1) | (s << 7);
            break;
         case 2: // RCL r/m8
            s = v & 0x80;
            v <<= 1;
            if (FLAGS(p->regs.flags, VXT_CARRY))
               v |= 1;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            break;
         case 3: // RCR r/m8
            s = v & 1;
            v >>= 1;
            if (FLAGS(p->regs.flags, VXT_CARRY))
               v |= 0x80;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            break;
         case 4: // SHL r/m8
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 0x80);
            v <<= 1;
            break;
         case 5: // SHR r/m8
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 1);
            v >>= 1;
            break;
         case 7: // SAR r/m8
            s = v & 0x80;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 1);
            v = (v >> 1) | s;
            break;
         default:
            return v; // Invalid opcode
		}
	}

	switch (p->mode.reg) {
      case 0:
         OVERFLOW_LEFT(v >> 7);
         break;
      case 1:
         OVERFLOW_RIGHT(v >> 6);
         break;
      case 2:
         OVERFLOW_LEFT(v >> 7);
         break;
      case 3:
         OVERFLOW_RIGHT(v >> 6);
         break;
      case 4:
         SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, (v >> 7) != (vxt_byte)(p->regs.flags & VXT_CARRY));
         flag_szp8(&p->regs, v);
         break;
      case 5:
         SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, (c == 1) && (o & 0x80));
         flag_szp8(&p->regs, v);
         break;
      case 7:
         p->regs.flags &= ~VXT_OVERFLOW;
         flag_szp8(&p->regs, v);
         break;
	}
	return v;
}

static vxt_word bitshift_16(CONSTSP(cpu) p, vxt_word v, vxt_byte c) {
	if (c == 0)
		return v;

   #ifdef VXT_CPU_286
	   c &= 0x1F;
   #endif

   vxt_word o = v;
   vxt_word s;

	for (int i = 0; i < c; i++) {
		switch (p->mode.reg) {
         case 0: // ROL r/m8
            s = v & 0x8000;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            v = (v << 1) | (s >> 15);
            break;
         case 1: // ROR r/m8
            s = v & 1;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            v = (v >> 1) | (s << 15);
            break;
         case 2: // RCL r/m8
            s = v & 0x8000;
            v <<= 1;
            if (FLAGS(p->regs.flags, VXT_CARRY))
               v |= 1;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            break;
         case 3: // RCR r/m8
            s = v & 1;
            v >>= 1;
            if (FLAGS(p->regs.flags, VXT_CARRY))
               v |= 0x8000;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, s);
            break;
         case 4: // SHL r/m8
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 0x8000);
            v <<= 1;
            break;
         case 5: // SHR r/m8
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 1);
            v >>= 1;
            break;
         case 7: // SAR r/m8
            s = v & 0x8000;
            SET_FLAG_IF(p->regs.flags, VXT_CARRY, v & 1);
            v = (v >> 1) | s;
            break;
         default:
            return v; // Invalid opcode
		}
	}

	switch (p->mode.reg) {
      case 0:
         OVERFLOW_LEFT(v >> 15);
         break;
      case 1:
         OVERFLOW_RIGHT(v >> 14);
         break;
      case 2:
         OVERFLOW_LEFT(v >> 15);
         break;
      case 3:
         OVERFLOW_RIGHT(v >> 14);
         break;
      case 4:
         SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, (v >> 15) != (vxt_byte)(p->regs.flags & VXT_CARRY));
         flag_szp16(&p->regs, v);
         break;
      case 5:
         SET_FLAG_IF(p->regs.flags, VXT_OVERFLOW, (c == 1) && (o & 0x8000));
         flag_szp16(&p->regs, v);
         break;
      case 7:
         p->regs.flags &= ~VXT_OVERFLOW;
         flag_szp16(&p->regs, v);
         break;
	}
	return v;
}

#undef OVERFLOW_LEFT
#undef OVERFLOW_RIGHT
