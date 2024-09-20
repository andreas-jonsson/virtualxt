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

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "common.h"

struct cpu;

enum exception_vector {
	CPU_DIV_ER_EXC      = 0,	// #DE Divide Error exception
	CPU_DEBUG_EXC       = 1,	// #DB Breakpoint/Single step interrupt
	CPU_NMI_INT         = 2,	//     NMI interrupt
	CPU_BREAKPOINT_INT  = 3,	// #BP INT3 (breakpoint) interrupt
	CPU_INTO_EXC        = 4,	// #OF INTO detected overflow exception
	CPU_BOUND_EXC       = 5,	// #BR BOUND range exceeded exception
	CPU_UD_EXC          = 6,	// #UD Undefined opcode exception
	CPU_NM_EXC          = 7,	// #NM NPX not available exception
	CPU_IDT_LIMIT_EXC   = 8,	//     Interrupt table limit too small exception
	CPU_DF_EXC          = 8,	// #DF Double Fault exception (pmode)
	CPU_NPX_SEG_OVR_INT = 9,	//     NPX segment overrun interrupt
	CPU_MP_EXC          = 9,	// #MP NPX protection fault exception (pmode)
	//                    10,	//     Reserved
	//                    11,	//     Reserved
	//                    12,	//     Reserved
	CPU_SEG_OVR_EXC     = 13,	//     Segment overrun exception (rmode)
	CPU_GP_EXC          = 13,	// #GP General Protection exception (pmode)
	//                    14,	//     Reserved
	//                    15	//     Reserved
	CPU_NPX_ERR_INT     = 16,	//     NPX error interrupt
	CPU_MF_EXC          = 16,	// #MF Math Fault exception (pmode)
	CPU_INVALID_EXC
};

#endif

