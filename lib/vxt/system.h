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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "common.h"
#include "cpu.h"

#define MAX_TIMERS 256
#define INT64 long long
#define PERIPHERAL_SIGNATURE 0xFAF129C3

 // Set this to 64K (not 64-16) because anything less causes problem with himem.sys.
#define EXT_MEM_SIZE 0x10000

#define VERIFY_PERIPHERAL(p, r)										\
	if (((struct peripheral*)(p))->sig != PERIPHERAL_SIGNATURE) {	\
		VXT_LOG("Invalid peripheral!");								\
		return r;													\
	}																\

struct timer {
   vxt_timer_id id;
   struct vxt_peripheral *dev;
   INT64 ticks;
   double interval;
};

struct peripheral {
    struct vxt_peripheral p;
    vxt_system *s;
    vxt_byte idx;
    vxt_dword sig;
    size_t size;
    // User device data is located at the end of this struct.
};

_Static_assert(sizeof(struct peripheral) % 4 == 0, "invalid struct size");

struct system {
   void *userdata;

   vxt_byte io_map[VXT_IO_MAP_SIZE];
   vxt_byte mem_map[VXT_MEM_MAP_SIZE];
   vxt_byte ext_mem[EXT_MEM_SIZE];

   vxt_allocator *alloc;
   struct cpu cpu;
   int frequency;
   bool a20;

   int num_timers;
   struct timer timers[MAX_TIMERS];

   int num_monitors;
   struct vxt_monitor monitors[VXT_MAX_MONITORS];

   int num_devices;
   struct vxt_peripheral *devices[VXT_MAX_PERIPHERALS];
   struct peripheral dummy;
};

void init_dummy_device(vxt_system *s);
vxt_byte system_in(vxt_system *s, vxt_word port);
void system_out(vxt_system *s, vxt_word port, vxt_byte data);

#endif
