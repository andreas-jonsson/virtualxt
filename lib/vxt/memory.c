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

#include "common.h"

struct memory {
    vxt_system *s;

    vxt_pointer base;
    bool read_only;

    int size;
    vxt_byte data[];
};

static vxt_byte read(void *d, vxt_pointer addr) {
    DEC_DEVICE(m, memory, d);
    ENSURE((int)(addr - m->base) < m->size);
    return m->data[addr - m->base];
}

static void write(void *d, vxt_pointer addr, vxt_byte data) {
    DEC_DEVICE(m, memory, d);
    ENSURE((int)(addr - m->base) < m->size);
    if (!m->read_only) {
        m->data[addr - m->base] = data;
    } else {
        LOG("writing to read-only memory: [0x%X] = 0x%X", addr, data);
    }
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *d) {
    DEC_DEVICE(m, memory, d->userdata);
    m->s = s;
    vxt_system_install_mem(s, d, m->base, (m->base + (vxt_pointer)m->size) - 1);
    return VXT_NO_ERROR;
}

static vxt_error destroy(void *d) {
    DEC_DEVICE(m, memory, d);
    vxt_system_allocator(m->s)(m, 0);
    return VXT_NO_ERROR;
}

struct vxt_pirepheral vxtu_create_memory_device(vxt_allocator *alloc, vxt_pointer base, int amount, bool read_only) {
    int size = sizeof(struct memory) + amount;
    struct memory *m = (struct memory*)alloc(NULL, size);
    memclear(m, size);

    m->base = base;
    m->read_only = read_only;
    m->size = amount;

    struct vxt_pirepheral p = {0};
    p.userdata = m;
    p.install = &install;
    p.io.read = &read;
    p.io.write = &write;
    return p;
}

bool vxtu_memory_device_fill(struct vxt_pirepheral *p, const vxt_byte *data, int size) {
    DEC_DEVICE(m, memory, p->userdata);
    ENSURE(data);
    if (m->size < size)
        return false;
    for (int i = 0; i < size; i++)
        m->data[i] = data[i];
    return true;
}
