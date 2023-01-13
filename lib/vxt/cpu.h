// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _CPU_H_
#define _CPU_H_

#include "common.h"

#define VALIDATOR_BEGIN(p, regs) { if ((p)->validator) (p)->validator->begin((regs), (p)->validator->userdata); }
#define VALIDATOR_END(p, name, op, mod, cycles, regs) { if ((p)->validator) (p)->validator->end((name), (op), (mod), (cycles), (regs), (p)->validator->userdata); }
#define VALIDATOR_READ(p, addr, data) { if ((p)->validator) (p)->validator->read((addr), (data), (p)->validator->userdata); }
#define VALIDATOR_WRITE(p, addr, data) { if ((p)->validator) (p)->validator->write((addr), (data), (p)->validator->userdata); }
#define VALIDATOR_DISCARD(p) { if ((p)->validator) (p)->validator->discard((p)->validator->userdata); }

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
   vxt_byte seg_override;

   void (*tracer)(vxt_system*,vxt_pointer,vxt_byte);
   const struct vxt_validator *validator;
   struct vxt_pirepheral *pic;
   vxt_system *s;
};

extern void cpu_reset(CONSTSP(cpu) p);
extern void cpu_reset_cycle_count(CONSTSP(cpu) p);
extern int cpu_step(CONSTSP(cpu) p);

#endif
