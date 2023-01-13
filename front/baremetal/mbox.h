// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef _MBOX_H_
#define _MBOX_H_

#include <vxt/vxt.h>

extern volatile vxt_dword mbox[39];
#define ADDR(X) (vxt_dword)((unsigned long)X)

// New tags for screen display
#define MBOX_TAG_SETPHYWH 0x48003
#define MBOX_TAG_SETVIRTWH 0x48004
#define MBOX_TAG_SETALPHA 0x48007
#define MBOX_TAG_SETVIRTOFF 0x48009
#define MBOX_TAG_SETDEPTH 0x48005
#define MBOX_TAG_SETPXLORDR 0x48006
#define MBOX_TAG_GETFB 0x40001
#define MBOX_TAG_GETPITCH 0x40008

// Registers
#define VIDEOCORE_MBOX (MMIO_BASE + 0xB880)
#define MBOX0_READ ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x00))
#define MBOX0_PEEK ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x10))
#define MBOX0_SENDER ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x14))
#define MBOX0_STATUS ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x18))
#define MBOX0_CONFIG ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x1C))
#define MBOX1_WRITE ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x20))
#define MBOX1_PEEK ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x30))
#define MBOX1_SENDER ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x34))
#define MBOX1_STATUS ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x38))
#define MBOX1_CONFIG ((volatile vxt_dword*)(VIDEOCORE_MBOX + 0x3C))

// Request/Response code in Buffer content
#define MBOX_RESPONSE 0x80000000
#define MBOX_REQUEST 0

// Status Value (from Status Register)
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

// Channels
#define MBOX_CH_POWER 0     // Power management
#define MBOX_CH_FB 1        // Frame buffer
#define MBOX_CH_VUART 2     // Virtual UART
#define MBOX_CH_VCHIQ 3     // VCHIQ
#define MBOX_CH_LEDS 4      // LEDs
#define MBOX_CH_BTNS 5      // Buttons
#define MBOX_CH_TOUCH 6     // Touch screen
#define MBOX_CH_PROP 8      // Property tags (ARM -> VC)

// Tags
#define MBOX_TAG_GETSERIAL 0x10004      // Get board serial
#define MBOX_TAG_GETMODEL 0x10001       // Get board serial
#define MBOX_TAG_SETCLKRATE 0x38002
#define MBOX_TAG_LAST 0

bool mbox_call(vxt_dword buffer_addr, vxt_byte channel);

#endif
