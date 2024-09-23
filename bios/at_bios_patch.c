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

bool patch_bios_call(vxt_system *s, struct vxt_registers *r, int n) {
	(void)s;
	if (n == 0x15) {
		switch (r->ah) {
			case 0x87:
				// Reference: https://www.powernet.co.za/info/bios/int/int15/87.htm
				//            https://www.stanislavs.org/helppc/int_15-87.html
				VXT_LOG("INT15,87: BIOS Request block from extended memory!");
				
				r->flags |= VXT_CARRY; // Set error
				r->ah = 2; // Just say interrupt error
				return true;
			case 0x88:
				r->ax = VXT_EXTENDED_MEMORY_SIZE * 1024; // Extended memory in KB
				r->flags &= ~VXT_CARRY; // No error
				VXT_LOG("INT15,88: Report %dKB of extended memory.", r->ax);
				return true;
			case 0x89:
				VXT_LOG("INT15,89: BIOS Request switch to protected mode!");
				return true;
		}
	}
	return false;
}
