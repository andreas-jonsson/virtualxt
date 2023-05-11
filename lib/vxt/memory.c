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

#include "common.h"
#include <vxt/vxtu.h>

VXT_PIREPHERAL_WITH_DATA(memory, vxt_byte, {
    vxt_pointer base;
    bool read_only;
    int size;
})

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(m, memory, p);
    ENSURE((int)(addr - m->base) < m->size);
    return VXT_GET_DEVICE_DATA(memory, p)[addr - m->base];
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(m, memory, p);
    ENSURE((int)(addr - m->base) < m->size);
    if (!m->read_only) {
        VXT_GET_DEVICE_DATA(memory, p)[addr - m->base] = data;
    } else {
        VXT_LOG("writing to read-only memory: [0x%X] = 0x%X", addr, data);
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(m, memory, p);
    vxt_system_install_mem(s, p, m->base, (m->base + (vxt_pointer)m->size) - 1);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(m, memory, p);
    return m->read_only ? "ROM" : "RAM";
}

struct vxt_pirepheral *vxtu_memory_create(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only) {
    int size = VXT_PIREPHERAL_SIZE(memory) + amount;
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, size);
    vxt_memclear(p, size);
    VXT_DEC_DEVICE(m, memory, p);

    #ifndef VXTU_MEMCLEAR
        if (!read_only) vxtu_randomize(VXT_GET_DEVICE_DATA(memory, p), amount, (long long int)p);
    #endif

    m->base = base;
    m->read_only = read_only;
    m->size = amount;   

    p->install = &install;
    p->name = &name;
    p->io.read = &read;
    p->io.write = &write;
    return p;
}

void *vxtu_memory_internal_pointer(struct vxt_pirepheral *p) {
    return VXT_GET_DEVICE_DATA(memory, p);
}

bool vxtu_memory_device_fill(struct vxt_pirepheral *p, const vxt_byte *data, int size) {
    VXT_DEC_DEVICE(m, memory, p);
    ENSURE(data);
    if (m->size < size)
        return false;
    for (int i = 0; i < size; i++)
        VXT_GET_DEVICE_DATA(memory, p)[i] = data[i];
    return true;
}
