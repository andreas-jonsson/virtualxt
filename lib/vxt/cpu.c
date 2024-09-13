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

#include "common.h"
#include "cpu.h"
#include "desc.h"
#include "testing.h"

#include "exec.inl"
#include "exec_ext.inl"
#include "optab.inl"

#ifndef VXT_NO_PREFETCH
   static void prefetch(CONSTSP(cpu) p, int num) {
      int i = 0;
      for (; i < num; i++) {
         if (p->inst_queue_count >= 6)
            return;

         vxt_pointer ptr = VXT_POINTER(p->sreg[VXT_SEGMENT_CS].raw, p->regs.ip + p->inst_queue_count);
         p->inst_queue_debug[p->inst_queue_count] = ptr;
         p->inst_queue[p->inst_queue_count++] = vxt_system_read_byte(p->s, ptr);
      }
      //if (i > 0) VXT_LOG("Prefetch %d bytes... %d", i, num);
   }
#endif

static void read_opcode(CONSTSP(cpu) p) {
   for (;;) {
      switch (p->opcode = read_opcode8(p)) {
         case 0x26:
            p->seg = VXT_SEGMENT_ES;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x2E:
            p->seg = VXT_SEGMENT_CS;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x36:
            p->seg = VXT_SEGMENT_SS;
            p->seg_override = p->opcode;
            p->cycles += 2;
            break;
         case 0x3E:
            p->seg = VXT_SEGMENT_DS;
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

static void prep_exec(CONSTSP(cpu) p) {
   p->inst_queue_dirty = false;
   p->bus_transfers = 0;

   bool trap = (p->regs.flags & VXT_TRAP) != 0;
   bool interrupt = (p->regs.flags & VXT_INTERRUPT) != 0;

   if (UNLIKELY(trap && !p->trap)) {
      p->trap = interrupt;
      call_int(p, 1);
   } else if (UNLIKELY(interrupt)) {
      p->halt = p->trap = false;
      if (LIKELY(p->pic != NULL)) {
         int n = p->pic->pic.next(VXT_GET_DEVICE_PTR(p->pic));
         if (n >= 0)
            call_int(p, n);
      }
   }

   // We need to do a direct reset in case we do interrupt.
   if (UNLIKELY(p->inst_queue_dirty)) {
      p->inst_queue_count = 0;
      p->inst_queue_dirty = false;
   }

   p->seg = VXT_SEGMENT_DS;
   p->seg_override = 0;
   p->repeat = 0;
   p->inst_start = p->regs.ip;
}

static void do_exec(CONSTSP(cpu) p) {
    const CONSTSP(instruction) inst = &opcode_table[p->opcode];
    ENSURE(inst->opcode == p->opcode);

    switch (inst->arch) {
        case ARCH_8086:
        case ARCH_80186:
        case ARCH_80286:
        case ARCH_FPU:
            p->invalid = false;
            break;
        default:
            p->invalid = true;
    }

    if (inst->modregrm)
        read_modregrm(p);
    inst->func(p, inst);
    
    if (p->invalid)
        call_int(p, 6);
    
    cpu_reflect_segment_registers(p);
    p->cycles += inst->cycles;

    if (UNLIKELY(p->inst_queue_dirty)) {
        p->inst_queue_count = 0;
    } else {
        #ifndef VXT_NO_PREFETCH
            prefetch(p, (p->cycles / 2) - p->bus_transfers); // TODO: Round up or down?
        #endif
    }
}

void cpu_reflect_segment_registers(CONSTSP(cpu) p) {
	p->regs.cs = p->sreg[VXT_SEGMENT_CS].raw;
	p->regs.ds = p->sreg[VXT_SEGMENT_DS].raw;
	p->regs.es = p->sreg[VXT_SEGMENT_ES].raw;
	p->regs.ss = p->sreg[VXT_SEGMENT_SS].raw;
}

int cpu_step(CONSTSP(cpu) p) {
    VALIDATOR_BEGIN(p, &p->regs);

    prep_exec(p);
    if (LIKELY(!p->halt)) {
        read_opcode(p);
        do_exec(p);
    } else {
        p->cycles++;
    }

    const CONSTSP(instruction) inst = &opcode_table[p->opcode];
    VALIDATOR_END(p, inst->name, p->opcode, inst->modregrm, p->cycles, &p->regs);

    ENSURE(p->cycles > 0);
    return p->cycles;
}

void cpu_reset_cycle_count(CONSTSP(cpu) p) {
   p->cycles = 0;
   p->int28 = false;
   p->invalid = false;
}

void cpu_reset(CONSTSP(cpu) p) {
	p->trap = false;
	vxt_memclear(&p->regs, sizeof(p->regs));

	p->regs.flags = 2;
	#ifdef FLAG8086
	   // 8086 flags
	   p->regs.flags |= 0xF000;
	#endif

	p->regs.debug = false;
	p->msw = 0;
	
	load_segment_register(p, VXT_SEGMENT_CS, 0xFFFF);

	p->inst_queue_count = 0;
	cpu_reset_cycle_count(p);
}

static vxt_byte read_byte(CONSTSP(cpu) p, vxt_pointer addr) {
   vxt_byte data = vxt_system_read_byte(p->s, addr);
   p->bus_transfers++;
   VALIDATOR_READ(p, addr, data);
   return data;
}

static void write_byte(CONSTSP(cpu) p, vxt_pointer addr, vxt_byte data) {
   vxt_system_write_byte(p->s, addr, data);
   p->bus_transfers++;
   VALIDATOR_WRITE(p, addr, data);
}

vxt_byte cpu_segment_read_byte(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word offset) {
	// TODO: Segment check here.
	return read_byte(p, DESCRIPTOR(p, seg)->base + offset);
}

vxt_word cpu_segment_read_word(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word offset) {
	// TODO: Segment check here.
	vxt_pointer base = DESCRIPTOR(p, seg)->base;
	return WORD(read_byte(p, base + ((offset + 1) & 0xFFFF)), read_byte(p, base + offset));
}

void cpu_segment_write_byte(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word offset, vxt_byte data) {
	// TODO: Segment check here.
	write_byte(p, DESCRIPTOR(p, seg)->base + offset, data);
}

void cpu_segment_write_word(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word offset, vxt_word data) {
	// TODO: Segment check here.
	vxt_pointer base = DESCRIPTOR(p, seg)->base;
	write_byte(p, base + offset, LBYTE(data));
	write_byte(p, base + ((offset + 1) & 0xFFFF), HBYTE(data));
}

bool cpu_is_protected(CONSTSP(cpu) p) {
	return (p->msw & MSW_PE) != 0;
}

TEST(register_layout,
   struct vxt_registers regs = {0};
   regs.ax = 0x0102;
   TENSURE(regs.ah == 1);
   TENSURE(regs.al == 2);

   regs.bx = 0x0304;
   TENSURE(regs.bh == 3);
   TENSURE(regs.bl == 4);
   TENSURE(regs.bx == 0x0304);

   // Make sure AX was not affected by BX.
   TENSURE(regs.ah == 1);
   TENSURE(regs.al == 2);
   TENSURE(regs.ax == 0x0102);

   regs.cx = 0x0506;
   TENSURE(regs.ch == 5);
   TENSURE(regs.cl == 6);
   TENSURE(regs.cx == 0x0506);

   regs.dx = 0x0708;
   TENSURE(regs.dh == 7);
   TENSURE(regs.dl == 8);
   TENSURE(regs.dx == 0x0708);
)
