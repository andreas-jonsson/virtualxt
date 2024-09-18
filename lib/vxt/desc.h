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

#ifndef _DESC_H_
#define _DESC_H_

#include "common.h"

#define DESCRIPTOR(p, seg) ( &(p)->sreg[(seg)].desc )
#define SELECTOR(p, seg) ( &(p)->sreg[(seg)].sel )

struct cpu;

struct segment_selector
{
	union {
		vxt_byte rpl;	// Requested Privilege Level
		vxt_byte cpl;	// Current Privilege Level
	};
	
	vxt_byte ti;		// Table indicator
	vxt_word index;		// Segment index
};

struct segment_descriptor {
	union {
		vxt_word limit;			// Segment / TSS
		vxt_word offset;		// Call Gate / Trap-Int Gate
	};
	
	union {
		vxt_word base_15_0;		// Segment / System / TSS
		vxt_word selector;		// Call Gate / Task Gate / Trap-Int Gate
	};
	
	union {
		vxt_word base_23_16;	// Segment / System / TSS
		vxt_word word_count;	// Call Gate
	};
	
	vxt_pointer base;
	bool valid;
	
	bool accessed;
	vxt_byte type;
	bool segment;
	vxt_byte dpl;
	bool present;
};

void load_segment_register(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word v);
UINT64 fetch_segment_descriptor(CONSTSP(cpu) p, enum vxt_segment seg, vxt_byte exvec);

#endif

