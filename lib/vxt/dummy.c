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

#include "common.h"
#include "system.h"

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    UNUSED(p);
    VXT_PRINT("reading unmapped IO port: %X\n", port);
    return 0xFF;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    UNUSED(p); UNUSED(data);
    VXT_PRINT("writing unmapped IO port: %X\n", port);
}

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    UNUSED(p);
    VXT_PRINT("reading unmapped memory: %X\n", addr);
    return 0xFF;
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    UNUSED(p); UNUSED(data);
    VXT_PRINT("writing unmapped memory: %X\n", addr);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io(s, p, 0x0, 0xFFFF);
    vxt_system_install_mem(s, p, 0x0, 0xFFFFF);
    return VXT_NO_ERROR;
}

void init_dummy_device(vxt_system *s) {
    struct _vxt_pirepheral *dummy = &((struct system*)s)->dummy;
    dummy->s = s;

    struct vxt_pirepheral *d = &dummy->p;
    d->install = &install;
    d->io.in = &in;
    d->io.out = &out;
    d->io.read = &read;
    d->io.write = &write;
}
