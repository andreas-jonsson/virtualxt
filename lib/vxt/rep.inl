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

#include "common.h"
#include "exec.h"

#define REPEAT(name, op)                                       \
   static void name (CONSTSP(cpu) p, INST(inst)) {             \
      UNUSED(inst);                                            \
      if (!p->repeat) {                                        \
         op                                                    \
         return;                                               \
      }                                                        \
      while (p->regs.cx) {                                     \
         op                                                    \
         p->regs.cx--;                                         \
         p->cycles++;                                          \
      }                                                        \
   }                                                           \

REPEAT(movsb_A4, {
   cpu_write_byte(p, VXT_POINTER(p->regs.es, p->regs.di), cpu_read_byte(p, VXT_POINTER(p->seg, p->regs.si)));
   update_di_si(p, 1);
})
REPEAT(movsw_A5, {
   cpu_write_word(p, VXT_POINTER(p->regs.es, p->regs.di), cpu_read_word(p, VXT_POINTER(p->seg, p->regs.si)));
   update_di_si(p, 2);
})
REPEAT(stosb_AA, {
   cpu_write_byte(p, VXT_POINTER(p->regs.es, p->regs.di), p->regs.al);
   update_di(p, 1);
})
REPEAT(stosw_AB, {
   cpu_write_word(p, VXT_POINTER(p->regs.es, p->regs.di), p->regs.ax);
   update_di(p, 2);
})
REPEAT(lodsb_AC, {
   p->regs.al = cpu_read_byte(p, VXT_POINTER(p->seg, p->regs.si));
   update_si(p, 1);
})
REPEAT(lodsw_AD, {
   p->regs.ax = cpu_read_word(p, VXT_POINTER(p->seg, p->regs.si));
   update_si(p, 2);
})
#undef REPEAT

#define REPEAT(name, op)                                                         \
   static void name (CONSTSP(cpu) p, INST(inst)) {                               \
      UNUSED(inst);                                                              \
      if (!p->repeat) {                                                          \
         op                                                                      \
         return;                                                                 \
      }                                                                          \
      while (p->regs.cx) {                                                       \
         op                                                                      \
         p->regs.cx--;                                                           \
         if (((p->repeat == 0xF3) && !(p->regs.flags & VXT_ZERO)) ||             \
            ((p->repeat == 0xF2) && (p->regs.flags & VXT_ZERO)))                 \
         {                                                                       \
            p->cycles += (p->repeat == 0xF2) ? 5 : 6;                            \
            break;                                                               \
         }                                                                       \
         p->cycles++;                                                            \
      }                                                                          \
   }                                                                             \

REPEAT(cmpsb_A6, {
   vxt_byte a = cpu_read_byte(p, VXT_POINTER(p->seg, p->regs.si));
   vxt_byte b = cpu_read_byte(p, VXT_POINTER(p->regs.es, p->regs.di));
   update_di_si(p, 1);
   flag_sub_sbb8(&p->regs, a, b, 0);
})
REPEAT(cmpsw_A7, {
   vxt_word a = cpu_read_word(p, VXT_POINTER(p->seg, p->regs.si));
   vxt_word b = cpu_read_word(p, VXT_POINTER(p->regs.es, p->regs.di));
   update_di_si(p, 2);
   flag_sub_sbb16(&p->regs, a, b, 0);
})
REPEAT(scasb_AE, {
   vxt_byte v = cpu_read_byte(p, VXT_POINTER(p->regs.es, p->regs.di));
   update_di(p, 1);
   flag_sub_sbb8(&p->regs, p->regs.al, v, 0);
})
REPEAT(scasw_AF, {
   vxt_word v = cpu_read_word(p, VXT_POINTER(p->regs.es, p->regs.di));
   update_di(p, 2);
   flag_sub_sbb16(&p->regs, p->regs.ax, v, 0);
})
#undef REPEAT

#define LOOP(name, cond, taken, ntaken)                  \
   static void name (CONSTSP(cpu) p, INST(inst)) {       \
      UNUSED(inst);                                      \
      vxt_word v = SIGNEXT16(read_opcode8(p));           \
      if (cond) {                                        \
         p->regs.ip += v;                                \
         p->inst_queue_dirty = true;                     \
         p->cycles += taken;                             \
      } else {                                           \
         p->cycles += ntaken;                            \
      }                                                  \
   }                                                     \
		
LOOP(loopnz_E0, --p->regs.cx && !(p->regs.flags & VXT_ZERO), 19, 5)
LOOP(loopz_E1, --p->regs.cx && (p->regs.flags & VXT_ZERO), 18, 6)
LOOP(loop_E2, --p->regs.cx, 17, 5)
#undef LOOP

static void jcxz_E3(CONSTSP(cpu) p, INST(inst)) {
   UNUSED(inst);
   vxt_word v = SIGNEXT16(read_opcode8(p));
   if (!p->regs.cx) {
      p->cycles += 12;
      p->regs.ip += v;
      p->inst_queue_dirty = true;
   }
}
