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

    vxt_byte opcode, repeat;
    bool wide_op, rm_to_reg;

    struct address_mode mode;

    vxt_word seg;
    bool seg_override;

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
