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

struct cpu;

struct segment_descriptor {
	vxt_pointer base;
	bool present;
	bool segment;
	bool valid;
	
	vxt_byte cpl;
};

void load_segment_register(CONSTSP(cpu) p, enum vxt_segment seg, vxt_word v);

#endif

