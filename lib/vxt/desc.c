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

#define TSS_BUSY_BIT   0x2
#define DESC_GATE_MASK 0x4

#define SEG_ACCESSED	0x1
#define SEG_READWRITE	0x2
#define SEG_CONFORMING	0x4
#define SEG_EXP_DOWN	0x4
#define SEG_EXECUTABLE	0x8
#define SEG_CODE		0x8
#define SEG_SEGMENT		0x10
#define SEG_PRESENT		0x80

#define SELECTOR_RPL_MASK 0xFFFC

void load_segment_register(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word v) {
	if (cpu_is_protected(p)) {
		struct segment_selector sel = {0};
		struct segment_descriptor desc = {0};
		
		sel.rpl = v & 3;
		sel.ti = (v >> 2) & 1;
		sel.index = v >> 3;
		
		p->sreg[seg].sel = sel;
		p->sreg[seg].desc = desc;
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
