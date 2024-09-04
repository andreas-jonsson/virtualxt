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
#include <vxt/vxtu.h>

struct memory {
    vxt_pointer base;
    bool read_only;
    int size;
    vxt_byte data[];
};

static vxt_byte read(struct memory *m, vxt_pointer addr) {
    ENSURE((int)(addr - m->base) < m->size);
    return m->data[addr - m->base];
}

static void write(struct memory *m, vxt_pointer addr, vxt_byte data) {
    ENSURE((int)(addr - m->base) < m->size);
    if (!m->read_only) {
        m->data[addr - m->base] = data;
    } else {
        VXT_LOG("writing to read-only memory: [0x%X] = 0x%X", addr, data);
    }
}

static vxt_error install(struct memory *m, vxt_system *s) {
    vxt_system_install_mem(s, VXT_GET_PERIPHERAL(m), m->base, (m->base + (vxt_pointer)m->size) - 1);
    return VXT_NO_ERROR;
}

static const char *name(struct memory *m) {
    return m->read_only ? "ROM" : "RAM";
}

VXT_API struct vxt_peripheral *vxtu_memory_create(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only) {
    int size = VXT_PERIPHERAL_SIZE(memory) + amount;
    struct VXT_PERIPHERAL(struct memory) *PERIPHERAL;
    *(void**)&PERIPHERAL = alloc(NULL, size);

    vxt_memclear(PERIPHERAL, size);
    struct memory *mem = VXT_GET_DEVICE(memory, PERIPHERAL);

    #ifndef VXTU_MEMCLEAR
        if (!read_only) vxtu_randomize(mem->data, amount, (intptr_t)PERIPHERAL);
    #endif

    mem->base = base;
    mem->read_only = read_only;
    mem->size = amount;

    PERIPHERAL->install = &install;
    PERIPHERAL->name = &name;
    PERIPHERAL->io.read = &read;
    PERIPHERAL->io.write = &write;

    return (struct vxt_peripheral*)PERIPHERAL;
}

VXT_API void *vxtu_memory_internal_pointer(struct vxt_peripheral *p) {
    return VXT_GET_DEVICE(memory, p)->data;
}

VXT_API bool vxtu_memory_device_fill(struct vxt_peripheral *p, const vxt_byte *data, int size) {
    struct memory *m = VXT_GET_DEVICE(memory, p);
    ENSURE(data);
    if (m->size < size)
        return false;
    for (int i = 0; i < size; i++)
        m->data[i] = data[i];
    return true;
}
