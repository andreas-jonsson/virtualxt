// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include "mbox.h"
#include "gpio.h"

void nop(void);

volatile vxt_dword __attribute__((aligned(16))) mbox[39];

static vxt_dword read(vxt_byte channel) {
	vxt_dword res;
	do {
		do nop(); while (*MBOX0_STATUS & MBOX_EMPTY);
		res = *MBOX0_READ;
	} while ((res & 0xF) != channel);
	return res;
}

static void send(vxt_dword msg) {
	do nop(); while (*MBOX1_STATUS & MBOX_FULL);
	*MBOX1_WRITE = msg;
}

bool mbox_call(vxt_dword buffer_addr, vxt_byte channel) {
	vxt_dword msg = (buffer_addr & ~0xF) | (channel & 0xF);
	send(msg);
	if (msg == read(channel)) {
		return mbox[1] == MBOX_RESPONSE;
	}
    return false;
}
