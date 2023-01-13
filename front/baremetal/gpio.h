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

#ifndef _GPIO_H_
#define _GPIO_H_

#define MMIO_BASE 0x3F000000	// Raspberry Pi 3's peripheral physical address

#define GPFSEL0         ((volatile unsigned int*)(MMIO_BASE + 0x00200000))
#define GPFSEL1         ((volatile unsigned int*)(MMIO_BASE + 0x00200004))
#define GPFSEL2         ((volatile unsigned int*)(MMIO_BASE + 0x00200008))
#define GPFSEL3         ((volatile unsigned int*)(MMIO_BASE + 0x0020000C))
#define GPFSEL4         ((volatile unsigned int*)(MMIO_BASE + 0x00200010))
#define GPFSEL5         ((volatile unsigned int*)(MMIO_BASE + 0x00200014))
#define GPSET0          ((volatile unsigned int*)(MMIO_BASE + 0x0020001C))
#define GPSET1          ((volatile unsigned int*)(MMIO_BASE + 0x00200020))
#define GPCLR0          ((volatile unsigned int*)(MMIO_BASE + 0x00200028))
#define GPLEV0          ((volatile unsigned int*)(MMIO_BASE + 0x00200034))
#define GPLEV1          ((volatile unsigned int*)(MMIO_BASE + 0x00200038))
#define GPEDS0          ((volatile unsigned int*)(MMIO_BASE + 0x00200040))
#define GPEDS1          ((volatile unsigned int*)(MMIO_BASE + 0x00200044))
#define GPHEN0          ((volatile unsigned int*)(MMIO_BASE + 0x00200064))
#define GPHEN1          ((volatile unsigned int*)(MMIO_BASE + 0x00200068))
#define GPPUD           ((volatile unsigned int*)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0       ((volatile unsigned int*)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1       ((volatile unsigned int*)(MMIO_BASE + 0x0020009C))

#endif
