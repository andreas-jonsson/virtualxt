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

static void extended_F(CONSTSP(cpu) p) {
    VALIDATOR_DISCARD(p);

    #ifdef TESTING

        // 8086 - pop cs
        p->regs.cs = pop(p);
        p->inst_queue_dirty = true;

    #else

   	switch (read_opcode8(p)) {
  		case 0:
 			read_modregrm(p);
 			switch (p->mode.reg) {
				case 0: // SLDT - Store Local Descriptor Table Register (pmode only)
   					VXT_LOG("SLDT - Store Local Descriptor Table Register");
   					rm_write16(p, 0);
   					return;
				case 1: // STR - Store Task Register (pmode only)
   					VXT_LOG("STR - Store Task Register");
   					return;
				case 2: // LDTR - Load Local Descriptor Table Register
   					VXT_LOG("LDTR - Load Local Descriptor Table Register");
   					return;
				case 3: // LTR - Load Task Register (pmode only)
   					VXT_LOG("LTR - Load Task Register");
   					return;
				case 4: // VERR - Verify a Segment for Reading (pmode only)
   					VXT_LOG("VERR - Verify a Segment for Reading");
   					return;
				case 5: // VERW - Verify a Segment for Writing (pmode only)
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
   					rm_write16(p, 0);
   					return;
				case 6: // LMSW - Load Machine Status Word
					{
						vxt_word msw = rm_read16(p);
						VXT_LOG("LMSW - Load Machine Status Word [%s%s%s%s]",
							(msw & 1) ? "PE, " : "", (msw & 2) ? "MP, " : "", (msw & 4) ? "EM, " : "", (msw & 8) ? "TS, " : "");
							
						if (msw & 1)
							VXT_LOG("WARNING: Enter protected mode on an XT machine will almost certainly result in system lockup.");
   					}
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
  		case 6: // CLTS - Clear Task-Switched Flag in CR0
 			VXT_LOG("CLTS - Clear Task-Switched Flag in CR0");
 			return;
   	}

    #endif
}

static void storeall_F1(CONSTSP(cpu) p) {
    VALIDATOR_DISCARD(p);

    if (read_opcode16(p) != 0xF04) {
        p->invalid = true;
        return;
    }

    // TODO: Implement
}
