// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _EXEC_H_
#define _EXEC_H_

#include "common.h"
#include "system.h"
#include "cpu.h"
#include "flags.h"

#define TRACE(p, ip, data) { if ((p)->tracer) (p)->tracer((p)->s, VXT_POINTER((p)->regs.cs, (ip)), (data)); }
#define IRQ(p, n) { VALIDATOR_DISCARD((p)); ENSURE((p)->pic); (p)->pic->pic.irq((p)->pic, (n)); }
#define INST(n) const struct instruction * const n

struct instruction {
   vxt_byte opcode;
   const char *name;
   bool modregrm;
   int cycles;
   void (*func)(CONSTSP(cpu), INST());
};

#define SIGNEXT16(v) signe_extend16(v)
#define SIGNEXT32(v) signe_extend32(v)

static vxt_dword signe_extend32(vxt_word v) {
   return (v & 0x8000) ? ((vxt_dword)v) | 0xFFFF0000 : (vxt_dword)v;
}

static vxt_word signe_extend16(vxt_byte v) {
   return (v & 0x80) ? ((vxt_word)v) | 0xFF00 : (vxt_word)v;
}

static vxt_byte read_opcode8(CONSTSP(cpu) p) {
   vxt_word ip = p->regs.ip;
   vxt_byte data = vxt_system_read_byte(p->s, VXT_POINTER(p->regs.cs, p->regs.ip++));
   TRACE(p, ip, data);
   return data;
}

static vxt_word read_opcode16(CONSTSP(cpu) p) {
   vxt_byte l = read_opcode8(p);
   vxt_byte h = read_opcode8(p);
   return WORD(h, l);
}

static vxt_pointer get_effective_address(CONSTSP(cpu) p) {
   CONSTSP(vxt_registers) r = &p->regs;
   CONSTSP(address_mode) m = &p->mode;
	vxt_pointer	ea = 0;
   int cycles = 0;

	switch (m->mod) {
      case 0:
         switch (m->rm) {
            case 0:
               ea = r->bx + r->si;
               cycles = 7;
               break;
            case 1:
               ea = r->bx + r->di;
               cycles = 8;
               break;
            case 2:
               ea = r->bp + r->si;
               cycles = 8;
               break;
            case 3:
               ea = r->bp + r->di;
               cycles = 7;
               break;
            case 4:
               ea = r->si;
               cycles = 5;
               break;
            case 5:
               ea = r->di;
               cycles = 5;
               break;
            case 6:
               ea = m->disp;
               cycles = 6;
               break;
            case 7:
               ea = r->bx;
               cycles = 5;
               break;
         }
         break;
      case 1:
      case 2:
         switch (m->rm) {
            case 0:
               ea = r->bx + r->si + m->disp;
               cycles = 11;
               break;
            case 1:
               ea = r->bx + r->di + m->disp;
               cycles = 12;
               break;
            case 2:
               ea = r->bp + r->si + m->disp;
               cycles = 12;
               break;
            case 3:
               ea = r->bp + r->di + m->disp;
               cycles = 11;
               break;
            case 4:
               ea = r->si + m->disp;
               cycles = 9;
               break;
            case 5:
               ea = r->di + m->disp;
               cycles = 9;
               break;
            case 6:
               ea = r->bp + m->disp;
               cycles = 9;
               break;
            case 7:
               ea = r->bx + m->disp;
               cycles = 9;
               break;
         }
         break;
	}
   p->ea_cycles = cycles;
	return VXT_POINTER(p->seg, ea);
}

static vxt_byte reg_read8(CONSTSP(vxt_registers) r, int reg) {
   switch (reg) {
      case 0:
         return r->al;
      case 1:
         return r->cl;
      case 2:
         return r->dl;
      case 3:
         return r->bl;
      case 4:
         return r->ah;
      case 5:
         return r->ch;
      case 6:
         return r->dh;
      case 7:
         return r->bh;
   }
   UNREACHABLE(0);
}

static vxt_word reg_read16(CONSTSP(vxt_registers) r, int reg) {
   switch (reg) {
      case 0:
         return r->ax;
      case 1:
         return r->cx;
      case 2:
         return r->dx;
      case 3:
         return r->bx;
      case 4:
         return r->sp;
      case 5:
         return r->bp;
      case 6:
         return r->si;
      case 7:
         return r->di;
   }
   UNREACHABLE(0);
}

static void reg_write8(CONSTSP(vxt_registers) r, int reg, vxt_byte data) {
   switch (reg) {
      case 0:
         r->al = data;
         return;
      case 1:
         r->cl = data;
         return;
      case 2:
         r->dl = data;
         return;
      case 3:
         r->bl = data;
         return;
      case 4:
         r->ah = data;
         return;
      case 5:
         r->ch = data;
         return;
      case 6:
         r->dh = data;
         return;
      case 7:
         r->bh = data;
         return;
      default:
         UNREACHABLE();
   }
}

static void reg_write16(CONSTSP(vxt_registers) r, int reg, vxt_word data) {
   switch (reg) {
      case 0:
         r->ax = data;
         return;
      case 1:
         r->cx = data;
         return;
      case 2:
         r->dx = data;
         return;
      case 3:
         r->bx = data;
         return;
      case 4:
         r->sp = data;
         return;
      case 5:
         r->bp = data;
         return;
      case 6:
         r->si = data;
         return;
      case 7:
         r->di = data;
         return;
      default:
         UNREACHABLE();
   }
}

static vxt_word seg_read16(CONSTSP(cpu) p) {
   CONSTSP(vxt_registers) r = &p->regs;
   switch (p->mode.reg & 3) {
		case 0:
         return r->es;
		case 1:
         return r->cs;
		case 2:
			return r->ss;
		case 3:
			return r->ds;
		default:
         UNREACHABLE(0); // Not sure what should happen here?
   }
}

static void seg_write16(CONSTSP(cpu) p, vxt_word data) {
   CONSTSP(vxt_registers) r = &p->regs;
   switch (p->mode.reg & 3) {
		case 0:
         r->es = data;
         return;
		case 1:
         r->cs = data;
         return;
		case 2:
			r->ss = data;
         return;
		case 3:
			r->ds = data;
         return;
		default:
         UNREACHABLE(); // Not sure what should happen here?
   }
}

#define RM_FUNC(a, b)                                                \
   static vxt_ ## a rm_read ## b (CONSTSP(cpu) p) {                  \
      if (p->mode.mod < 3) {                                         \
         vxt_pointer ea = get_effective_address(p);                  \
         return vxt_system_read_ ## a (p->s, ea);                    \
      } else {                                                       \
         return reg_read ## b (&p->regs, p->mode.rm);                \
      }                                                              \
   }                                                                 \
                                                                     \
   static void rm_write ## b (CONSTSP(cpu) p, vxt_ ## a data) {      \
      if (p->mode.mod < 3) {                                         \
         vxt_pointer ea = get_effective_address(p);                  \
         vxt_system_write_ ## a (p->s, ea, data);                    \
      } else {                                                       \
         reg_write ## b (&p->regs, p->mode.rm, data);                \
      }                                                              \
   }                                                                 \

#define NARROW(f) f(byte, 8)
#define WIDE(f) f(word, 16)

NARROW(RM_FUNC)
WIDE(RM_FUNC)

#undef RM_FUNC
#undef NARROW
#undef WIDE

static void push(CONSTSP(cpu) p, vxt_word data) {
   p->regs.sp -= 2;
   vxt_system_write_word(p->s, VXT_POINTER(p->regs.ss, p->regs.sp), data);
}

static vxt_word pop(CONSTSP(cpu) p) {
   vxt_word data = vxt_system_read_word(p->s, VXT_POINTER(p->regs.ss, p->regs.sp));
   p->regs.sp += 2;
   return data;
}

static void update_di(CONSTSP(cpu) p, vxt_word n) {
   p->regs.di += (p->regs.flags & VXT_DIRECTION) ? -n : n;
}

static void update_si(CONSTSP(cpu) p, vxt_word n) {
   p->regs.si += (p->regs.flags & VXT_DIRECTION) ? -n : n;
}

static void update_di_si(CONSTSP(cpu) p, vxt_word n) {
   if (p->regs.flags & VXT_DIRECTION) {
      p->regs.si -= n;
      p->regs.di -= n;
   } else {
      p->regs.si += n;
      p->regs.di += n;
   }
}

static void override_with_ss(CONSTSP(cpu) p, bool cond) {
   if (!p->seg_override && cond)
	   p->seg = p->regs.ss;
}

static vxt_byte read_modregrm(CONSTSP(cpu) p) {
   vxt_byte modregrm = read_opcode8(p);
   struct address_mode mode = {
      .mod = modregrm >> 6,
      .reg = (modregrm >> 3) & 7,
      .rm = modregrm & 7,
      .disp = 0
   };
   
	switch(mode.mod) {
	   case 0:
	      if (mode.rm == 6)
	         mode.disp = read_opcode16(p);
         override_with_ss(p, (mode.rm == 2) || (mode.rm == 3));
	      break;
	   case 1:
	      mode.disp = SIGNEXT16(read_opcode8(p));
         override_with_ss(p, (mode.rm == 2) || (mode.rm == 3) || (mode.rm == 6));
	      break;
	   case 2:
	      mode.disp = read_opcode16(p);
	      override_with_ss(p, (mode.rm == 2) || (mode.rm == 3) || (mode.rm == 6));
	      break;
	}

   p->mode = mode;
   return modregrm;
}

static void call_int(CONSTSP(cpu) p, int n) {
   VALIDATOR_DISCARD(p);
   CONSTSP(vxt_registers) r = &p->regs;

   #ifdef VXT_CPU_286
      push(p, (r->flags & ALL_FLAGS) | 0x2);
   #else
      push(p, (r->flags & ALL_FLAGS) | 0xF002);
   #endif

   push(p, r->cs);
   push(p, r->ip);

   r->ip = vxt_system_read_word(p->s, (vxt_pointer)n * 4);
   r->cs = vxt_system_read_word(p->s, (vxt_pointer)n * 4 + 2);
   r->flags &= ~(VXT_INTERRUPT|VXT_TRAP);
}

static void divZero(CONSTSP(cpu) p) {
   p->regs.ip = p->inst_start;
   call_int(p, 0);
}

static void prep_exec(CONSTSP(cpu) p) {
   if (p->trap)
      call_int(p, 1);

   p->trap = (p->regs.flags & VXT_TRAP) != 0;
   if (p->pic && !p->trap && (p->regs.flags & VXT_INTERRUPT)) {
      int n = p->pic->pic.next(p->pic);
      if (n >= 0) {
         p->halt = false;
         call_int(p, n);
      }
   }

   p->seg = p->regs.ds;
   p->seg_override = 0;
   p->repeat = 0;
   p->inst_start = p->regs.ip;
}

static bool valid_repeat(vxt_byte opcode) {
   if ((opcode >= 0xA4) && (opcode <= 0xA7))
      return true;
   if ((opcode >= 0xAA) && (opcode <= 0xAF))
      return true;
   #if defined(VXT_CPU_286) || defined(VXT_CPU_V20)
      if ((opcode >= 0x6C) && (opcode <= 0x6F))
         return true;
   #endif
   return false;
}

static void read_opcode(CONSTSP(cpu) p) {
   for (;;) {
      switch (p->opcode = read_opcode8(p)) {
         case 0x26:
            p->seg = p->regs.es;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x2E:
            p->seg = p->regs.cs;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x36:
            p->seg = p->regs.ss;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x3E:
            p->seg = p->regs.ds;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0xF2: // REPNE/REPNZ
         case 0xF3: // REP/REPE/REPZ
            p->repeat = p->opcode;
            p->cycles += 2;
            break;
         default:
            if (p->repeat && !valid_repeat(p->opcode))
               p->repeat = 0;
            return;
      }
   }
}

#endif