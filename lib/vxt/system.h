/*
Copyright (c) 2019-2022 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "common.h"
#include "cpu.h"

#define MAX_DEVICES 256

struct system {
   void *userdata;

   vxt_byte io_map[0x10000];
   vxt_byte mem_map[0x100000];

   vxt_allocator *alloc;
   struct cpu cpu;

   int num_devices;
   struct vxt_pirepheral devices[MAX_DEVICES];
};

extern void init_dummy_device(vxt_system *s, struct vxt_pirepheral *d);    // From dummy.c
extern vxt_byte system_in(vxt_system *s, vxt_word port);
extern void system_out(vxt_system *s, vxt_word port, vxt_byte data);

#endif
