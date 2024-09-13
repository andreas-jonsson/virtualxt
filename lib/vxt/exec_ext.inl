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

static void extended_F(CONSTSP(cpu) p, INST(inst)) {
    VALIDATOR_DISCARD(p);
    UNUSED(inst);

    #ifdef TESTING

        // 8086 - pop cs
        load_segment_register(p, VXT_SEGMENT_CS, pop(p));
        p->inst_queue_dirty = true;

    #else

   	switch (read_opcode8(p)) {
  		case 0:
 			read_modregrm(p);
 			switch (p->mode.reg) {
				case 0: // SLDT - Store Local Descriptor Table Register
   					VXT_LOG("SLDT - Store Local Descriptor Table Register");
   					return;
				case 1: // STR - Store Task Register
   					VXT_LOG("STR - Store Task Register");
   					return;
				case 2: // LDTR - Load Local Descriptor Table Register
   					VXT_LOG("LDTR - Load Local Descriptor Table Register");
   					return;
				case 3: // LTR - Load Task Register
   					VXT_LOG("LTR - Load Task Register");
   					return;
				case 4: // VERR - Verify a Segment for Reading
   					VXT_LOG("VERR - Verify a Segment for Reading");
   					return;
				case 5: // VERW - Verify a Segment for Writing
   					VXT_LOG("VERW - Verify a Segment for Writing");
   					return;
 			}
 			return;
  		case 1:
 			read_modregrm(p);
 			switch (p->mode.reg) {
				case 0: // SGDT - Store Global Descriptor Table Register
   					VXT_LOG("SGDT - Store Global Descriptor Table Register");
   					return;
				case 1: // SIDT - Store Interrupt Descriptor Table Register
   					VXT_LOG("SIDT - Store Interrupt Descriptor Table Register");
   					return;
				case 2: // LGDT - Load Global Descriptor Table Register
   					VXT_LOG("LGDT - Load Global Descriptor Table Register");
   					return;
				case 3: // LIDT - Load Interrupt Descriptor Table Register
   					VXT_LOG("LIDT - Load Interrupt Descriptor Table Register");
   					return;
				case 4: // SMSW - Store Machine Status Word
   					VXT_LOG("SMSW - Store Machine Status Word");
   					return;
				case 6: // LMSW - Load Machine Status Word
   					VXT_LOG("LMSW - Load Machine Status Word");
   					return;
 			}
 			return;
  		case 2: // LAR - Load Access Rights Byte
 			VXT_LOG("LAR - Load Access Rights Byte");
 			read_modregrm(p);
 			return;
  		case 3: // LSL - Load Segment Limit
 			VXT_LOG("LSL - Load Segment Limit");
 			read_modregrm(p);
 			return;
  		case 5: // LOADALL - Load All of the CPU Registers
 			VXT_LOG("LOADALL - Load All of the CPU Registers");
 			return;
  		case 6: // CLTS - Clear Task-Switched Flag in MSW
 			VXT_LOG("CLTS - Clear Task-Switched Flag in MSW");
            p->msw &= ~MSW_TS;
 			return;
   	}

    #endif
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
