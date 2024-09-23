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

#include "desc.h"
#include "cpu.h"
#include "exception.h"

#define DESC_TYPE_INVALID		0x0
#define DESC_TYPE_AVAIL_TSS		0x1
#define DESC_TYPE_LDT_DESC		0x2
#define DESC_TYPE_BUSY_TSS		0x3
#define DESC_TYPE_CALL_GATE		0x4
#define DESC_TYPE_TASK_GATE		0x5
#define DESC_TYPE_INTR_GATE		0x6
#define DESC_TYPE_TRAP_GATE		0x7

#define TSS_BUSY_BIT	0x2
#define DESC_GATE_MASK	0x4

#define SEG_ACCESSED	0x1
#define SEG_READWRITE	0x2
#define SEG_CONFORMING	0x4
#define SEG_EXP_DOWN	0x4
#define SEG_EXECUTABLE	0x8
#define SEG_CODE		0x8
#define SEG_SEGMENT		0x10
#define SEG_PRESENT		0x80

#define SEG_TYPE_READWRITE	0x1
#define SEG_TYPE_READABLE	0x1
#define SEG_TYPE_WRITABLE	0x1
#define SEG_TYPE_CONFORMING	0x2
#define SEG_TYPE_EXP_DOWN	0x2
#define SEG_TYPE_EXECUTABLE	0x4
#define SEG_TYPE_CODE		0x4

#define SELECTOR_RPL_MASK 0xFFFC

void load_segment_register(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word v) {
	if (cpu_is_protected(p)) {
		struct segment_selector sel = {0};
		struct segment_descriptor desc = {0};
		
		if (seg == VXT_SEGMENT_SS) {
			if ((v & SELECTOR_RPL_MASK) == 0) {
				VXT_LOG("load_segment_register: (v & SELECTOR_RPL_MASK) == 0");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			// Update selector
			sel.rpl = v & 3;
			sel.ti = (v >> 2) & 1;
			sel.index = v >> 3;
			
			if (sel.rpl != p->sreg[VXT_SEGMENT_CS].sel.cpl) {
				VXT_LOG("load_segment_register: sel.rpl != p->sreg[VXT_SEGMENT_CS].sel.cpl");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			load_segment_descriptor(&desc, fetch_segment_descriptor(p, seg, CPU_GP_EXC));
			
			if (!desc.valid) {
				VXT_LOG("load_segment_register: !desc->valid");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			bool is_data = desc.segment && !(desc.type & SEG_TYPE_CODE);
			bool is_writeable = desc.segment && (desc.type & SEG_TYPE_WRITABLE);
			
			if (!is_data || !is_writeable) {
				VXT_LOG("load_segment_register: !is_data || !is_writeable");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			if (desc.dpl != p->sreg[VXT_SEGMENT_CS].sel.cpl) {
				VXT_LOG("load_segment_register: desc->dpl != p->sreg[VXT_SEGMENT_CS].sel.cpl");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			if (!desc.present) {
				VXT_LOG("load_segment_register: !desc->present");
				cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
			}
			
			p->sreg[seg].sel = sel;
			p->sreg[seg].desc = desc;
			update_segment_descriptor(p, seg);
		} else if ((seg == VXT_SEGMENT_DS) || (seg == VXT_SEGMENT_ES)) {
			sel.rpl = v & 3;
			sel.ti = (v >> 2) & 1;
			sel.index = v >> 3;
				
			if ((v & SELECTOR_RPL_MASK) == 0) {
				desc.valid = false;
				set_descriptor_access(&desc, SEG_SEGMENT);
			} else {
				load_segment_descriptor(&desc, fetch_segment_descriptor(p, seg, CPU_GP_EXC));
			
				if (!desc.valid) {
					VXT_LOG("load_segment_register: !desc.valid");
					cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
				}

				bool is_code_segment = desc.segment && (desc.type & SEG_TYPE_CODE);
				bool is_readable = desc.segment && (desc.type & SEG_TYPE_READABLE);
				
				if (!desc.segment || (is_code_segment && !is_readable)) {
					VXT_LOG("load_segment_register: is_system_segment || (is_code_segment && !is_readable)");
					cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
				}
				
				bool is_data_segment = desc.segment && !(desc.type & SEG_TYPE_CODE);

				if (is_data_segment && ((sel.rpl > desc.dpl) || (p->sreg[VXT_SEGMENT_CS].sel.cpl > desc.dpl))) {
					VXT_LOG("load_segment_register: is_data_segment && ((sel.rpl > desc.dpl) || (p->sreg[VXT_SEGMENT_CS].sel.cpl > desc.dpl)");
					cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
				}

				if (!desc.present) {
					VXT_LOG("load_segment_register: !desc.present");
					cpu_throw_exception(p, CPU_GP_EXC, v & SELECTOR_RPL_MASK);
				}
			
				p->sreg[seg].sel = sel;
				p->sreg[seg].desc = desc;
				update_segment_descriptor(p, seg);
			}
		} else {
			PANIC("load_segment_register: invalid segment register");
		}
	} else {
		struct segment_descriptor *desc = DESCRIPTOR(p, seg);
		
		desc->base = ((vxt_pointer)v) << 4;
		desc->present = true;
		desc->segment = true;
		desc->valid = true;
		
		SELECTOR(p, seg)->cpl = 0;
	}

	p->sreg[seg].raw = v;
}

vxt_byte get_descriptor_access(struct segment_descriptor *desc) {
	vxt_byte ar = desc->segment << 4 | desc->dpl << 5 | desc->present << 7;
	return ar | (desc->segment ? desc->type << 1 | desc->accessed : desc->type);
}

void set_descriptor_access(struct segment_descriptor *desc, vxt_byte ar) {
	desc->valid = true;
	desc->segment = ar & SEG_SEGMENT;
	desc->type = (ar >> 1) & 7;
	
	if (desc->segment) 	// Code or Data Segment Descriptor
		desc->accessed = ar & 1;
	else 				// System Segment Descriptor or Gate Descriptor
		desc->valid = desc->type == DESC_TYPE_INVALID;
	
	desc->dpl = (ar >> 5) & 3;
	desc->present = ar & 0x80;
}

void load_segment_defaults(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word v) {
	ENSURE(!cpu_is_protected(p));
	
	p->sreg[seg].raw = v;
	SELECTOR(p, seg)->cpl = 0;

	struct segment_descriptor *desc = DESCRIPTOR(p, seg);
	desc->base = ((vxt_pointer)v) << 4;
	desc->limit = 0xFFFF;
	desc->dpl = 0;

	switch (seg) {
		case _VXT_REG_LDTR:
			set_descriptor_access(desc, SEG_PRESENT | DESC_TYPE_LDT_DESC);
			break;
		case _VXT_REG_TR:
			set_descriptor_access(desc, SEG_PRESENT);
			break;
		default:
			set_descriptor_access(desc, SEG_SEGMENT | SEG_PRESENT | SEG_ACCESSED | SEG_READWRITE);
	}
}

void load_segment_descriptor(struct segment_descriptor *desc, uint64_t v) {
	set_descriptor_access(desc, (vxt_byte)(v >> 40));

	if (desc->segment || !(desc->type & DESC_GATE_MASK)) {
		desc->limit = v & 0xFFFF;
		desc->base_15_0 = v >> 16;
		desc->base_23_16 = v >> 32;
		desc->base = ((vxt_pointer)(desc->base_23_16) << 16) | desc->base_15_0;
	} else {
		// Call gates only
		desc->offset = v & 0xFFFF;
		desc->selector = v >> 16;
		desc->word_count = (v >> 32) & 0x1F;
	}
}

UINT64 fetch_segment_descriptor(CONSTSP(cpu) p, enum vxt_segment seg, vxt_byte exvec) {
	struct segment_selector *sel = SELECTOR(p, seg);
	vxt_word offset = sel->index * 8;
	enum vxt_segment table;
	
	if (sel->ti) { // LDT
		table = _VXT_REG_LDTR;
		if (!DESCRIPTOR(p, table)->valid)
			cpu_throw_exception(p, exvec, p->sreg[seg].raw & SELECTOR_RPL_MASK);
	} else { // GDT
		table = _VXT_REG_GDTR;
	}
	
	struct segment_descriptor *desc = DESCRIPTOR(p, table);
	if ((offset + 7u) > desc->limit)
		cpu_throw_exception(p, exvec, p->sreg[seg].raw & SELECTOR_RPL_MASK);

	vxt_pointer addr = desc->base + offset;
	return ((UINT64)vxt_system_read_word(p->s, addr + 2) << 32) | (UINT64)vxt_system_read_word(p->s, addr);
}

void update_segment_descriptor(CONSTSP(cpu) p, enum vxt_segment seg) {
	struct segment_selector *sel = SELECTOR(p, seg);
	struct segment_descriptor *desc = DESCRIPTOR(p, seg);
	
	if (!desc->accessed) {
		desc->accessed = true;
		vxt_byte ar = get_descriptor_access(desc);
		enum vxt_segment r = sel->ti ? _VXT_REG_LDTR : _VXT_REG_GDTR;

		vxt_system_write_byte(p->s, DESCRIPTOR(p, r)->base + sel->index * 8 + 5, ar);
	}
}
