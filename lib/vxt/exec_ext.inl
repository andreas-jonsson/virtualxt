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
#include "exec.h"

static void ext_sldt(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("SLDT - Store Local Descriptor Table Register");
}

static void ext_str(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("STR - Store Task Register");
}

static void ext_ldtr(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("LDTR - Load Local Descriptor Table Register");
}

static void ext_ltr(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("LTR - Load Task Register");
}

static void ext_verr(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("VERR - Verify a Segment for Reading");
}

static void ext_verw(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	UNUSED(p);
	VXT_LOG("VERW - Verify a Segment for Writing");
}

static void ldt(CONSTSP(cpu) p, enum vxt_segment table) {
	// TODO: Check pmode
	
	struct segment_descriptor *desc = DESCRIPTOR(p, table);
	vxt_word offset = get_ea_offset(p);
	
	desc->limit = cpu_segment_read_word(p, p->seg, offset);
	desc->base = (((vxt_pointer)cpu_segment_read_word(p, p->seg, (offset + 4) & 0xFFFF) << 16) | (vxt_pointer)cpu_segment_read_word(p, p->seg, (offset + 2) & 0xFFFF)) & 0xFFFFFF;
}

static void sdt(CONSTSP(cpu) p, enum vxt_segment table) {
	// TODO: Check pmode
	
	struct segment_descriptor *desc = DESCRIPTOR(p, table);
	vxt_pointer base = desc->base | 0xFF000000;
	vxt_word offset = get_ea_offset(p);
		
	cpu_segment_write_word(p, p->seg, offset, desc->limit);
	cpu_segment_write_word(p, p->seg, (offset + 2) & 0xFFFF, base & 0xFFFF);
	cpu_segment_write_word(p, p->seg, (offset + 4) & 0xFFFF, base >> 16);
}

static void ext_lar(CONSTSP(cpu) p, INST(inst)) {
	UNUSED(inst);
	VXT_LOG("LAR - Load Access Rights Byte");
	read_modregrm(p);
}

static void extended_F(CONSTSP(cpu) p, INST(inst)) {
    VALIDATOR_DISCARD(p);
    
    #ifdef TESTING
		UNUSED(inst);

        // 8086 - pop cs
        load_segment_register(p, VXT_SEGMENT_CS, pop(p));
        p->inst_queue_dirty = true;

    #else

   	switch (read_opcode8(p)) {
  		case 0:
 			read_modregrm(p);
 			switch (p->mode.reg) {
				case 0: // SLDT - Store Local Descriptor Table Register
   					ext_sldt(p, inst);
   					return;
				case 1: // STR - Store Task Register
   					ext_str(p, inst);
   					return;
				case 2: // LDTR - Load Local Descriptor Table Register
   					ext_ldtr(p, inst);
   					return;
				case 3: // LTR - Load Task Register
   					ext_ltr(p, inst);
   					return;
				case 4: // VERR - Verify a Segment for Reading
   					ext_verr(p, inst);
   					return;
				case 5: // VERW - Verify a Segment for Writing
   					ext_verw(p, inst);
   					return;
 			}
 			return;
  		case 1:
 			read_modregrm(p);
 			switch (p->mode.reg) {
				case 0: // SGDT - Store Global Descriptor Table Register
   					sdt(p, _VXT_REG_GDTR);
   					return;
				case 1: // SIDT - Store Interrupt Descriptor Table Register
   					sdt(p, _VXT_REG_IDTR);
   					return;
				case 2: // LGDT - Load Global Descriptor Table Register
   					ldt(p, _VXT_REG_GDTR);
   					return;
				case 3: // LIDT - Load Interrupt Descriptor Table Register
   					ldt(p, _VXT_REG_IDTR);
   					return;
				case 4: // SMSW - Store Machine Status Word
   					rm_write16(p, p->msw);
   					return;
				case 6: // LMSW - Load Machine Status Word
   					{
						vxt_word msw = rm_read16(p);
						if (cpu_is_protected(p) && !(msw & MSW_PE)) {
							VXT_LOG("CPU enter real mode!");
							load_segment_defaults(p, VXT_SEGMENT_CS, p->sreg[VXT_SEGMENT_CS].raw);
						} else if (!cpu_is_protected(p) && (msw & MSW_PE)) {
							VXT_LOG("CPU enter protected mode!");
						}
						p->msw = msw;
					}
   					return;
 			}
 			return;
  		case 2: // LAR - Load Access Rights Byte
 			ext_lar(p, inst);
 			return;
  		case 3: // LSL - Load Segment Limit
 			VXT_LOG("LSL - Load Segment Limit");
 			read_modregrm(p);
 			return;
  		case 5: // LOADALL - Load All of the CPU Registers
 			VXT_LOG("LOADALL - Load All of the CPU Registers");
 			return;
  		case 6: // CLTS - Clear Task-Switched Flag in MSW
 			// TODO: Check pmode
            p->msw &= ~MSW_TS;
 			return;
 		default:
			p->invalid = true;
   	}

    #endif
}

static void arpl_63(CONSTSP(cpu) p, INST(inst)) {
   VALIDATOR_DISCARD(p);
   UNUSED(inst);

   // TODO: Implement
   VXT_LOG("Warning! ARPL is not implemented!");
   p->regs.flags &= ~VXT_ZERO;
}

static void storeall_F1(CONSTSP(cpu) p, INST(inst)) {
    VALIDATOR_DISCARD(p);
    UNUSED(inst);

    if (read_opcode16(p) != 0xF04) {
        p->invalid = true;
        return;
    }

    // TODO: Implement
}
