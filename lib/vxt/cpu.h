// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _CPU_H_
#define _CPU_H_

#include "common.h"

// TODO: This need some more work.
#if !defined(VXT_NO_PREFETCH) && defined(PI8088)
   #define VXT_NO_PREFETCH
#endif

#ifndef VXT_NO_PREFETCH
   //#define VXT_DEBUG_PREFETCH
#endif

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
   bool trap, halt, int28, invalid;
   int cycles;
   vxt_word inst_start;

   vxt_byte opcode, repeat;
   struct address_mode mode;

   vxt_word seg;
   vxt_byte seg_override;

   int bus_transfers;

   bool inst_queue_dirty;
   int inst_queue_count;
   vxt_byte inst_queue[6];
   vxt_pointer inst_queue_debug[6];

   void (*tracer)(vxt_system*,vxt_pointer,vxt_byte);
   const struct vxt_validator *validator;
   struct vxt_peripheral *pic;
   vxt_system *s;
};

vxt_byte cpu_read_byte(CONSTSP(cpu) p, vxt_pointer addr);
void cpu_write_byte(CONSTSP(cpu) p, vxt_pointer addr, vxt_byte data);
vxt_word cpu_read_word(CONSTSP(cpu) p, vxt_pointer addr);
vxt_word cpu_segment_read_word(CONSTSP(cpu) p, vxt_word segment, vxt_word offset);
void cpu_write_word(CONSTSP(cpu) p, vxt_pointer addr, vxt_word data);
void cpu_segment_write_word(CONSTSP(cpu) p, vxt_word segment, vxt_word offset, vxt_word data);
void cpu_reset(CONSTSP(cpu) p);
void cpu_reset_cycle_count(CONSTSP(cpu) p);
int cpu_step(CONSTSP(cpu) p);

#endif
