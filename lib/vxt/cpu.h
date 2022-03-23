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

#ifndef _CPU_H_
#define _CPU_H_

#include "common.h"

#define ALL_FLAGS (VXT_CARRY | VXT_PARITY | VXT_AUXILIARY | VXT_ZERO | VXT_SIGN | VXT_TRAP | VXT_INTERRUPT | VXT_DIRECTION | VXT_OVERFLOW)
#define FLAGS(r, f) ( ((r) & (f)) == (f) )
#define ANY_FLAGS(r, f) ( ((r) & (f)) != 0 )
#define SET_FLAG(r, f, v) ( (r) = ((r) & ~(f)) | ((v) & (f)) )
#define SET_FLAG_IF(r, f, c) ( SET_FLAG((r), (f), (c) ? (f) : ~(f)) )

#define VALIDATOR_BEGIN(p, name, op, mod, regs) { if ((p)->validator) (p)->validator->begin((name), (op), (mod), (regs), (p)->validator->userdata); }
#define VALIDATOR_END(p, cycles, regs) { if ((p)->validator) (p)->validator->end((cycles), (regs), (p)->validator->userdata); }
#define VALIDATOR_READ(p, addr, data) { if ((p)->validator) (p)->validator->read((addr), (data), (p)->validator->userdata); }
#define VALIDATOR_WRITE(p, addr, data) { if ((p)->validator) (p)->validator->write((addr), (data), (p)->validator->userdata); }
#define VALIDATOR_DISCARD(p) { if ((p)->validator) (p)->validator->discard((p)->validator->userdata); }

#define TRACE(p, ip, data) { if ((p)->tracer) (p)->tracer((p)->s, VXT_POINTER((p)->regs.cs, (ip)), (data)); }

#define IRQ(p, n) { VALIDATOR_DISCARD((p)); ENSURE((p)->pic); (p)->pic->pic.irq((p)->pic, (n)); }

#define INST(n) const struct instruction * const n

struct address_mode {
   vxt_byte mod, reg, rm;
   vxt_word disp;
};

struct cpu {
   struct vxt_registers regs;
   bool trap, halt;
   int cycles, ea_cycles;
   vxt_word inst_start;

   int opcode_zero_count;

   vxt_byte opcode, repeat;
   struct address_mode mode;

   vxt_word seg;
   bool seg_override;
   bool has_prefix;

   void (*tracer)(vxt_system*,vxt_pointer,vxt_byte);
   const struct vxt_validator *validator;
   struct vxt_pirepheral *pic;
   vxt_system *s;
};

struct instruction {
   vxt_byte opcode;
   const char *name;
   bool modregrm;
   int cycles;
   void (*func)(CONSTSP(cpu), INST());
};

extern void cpu_reset(CONSTSP(cpu) p);
extern void cpu_reset_cycle_count(CONSTSP(cpu) p);
extern int cpu_step(CONSTSP(cpu) p);

#endif
