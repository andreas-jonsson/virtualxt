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

#ifdef NDEBUG
    #define VXT_NO_LOG
#endif

#include "common.h"
#include "system.h"

static vxt_byte in(void *p, vxt_word port) {
    UNUSED(p); UNUSED(port);
    VXT_PRINT("reading unmapped IO port: %X\n", port);
    return 0xFF;
}

static void out(void *p, vxt_word port, vxt_byte data) {
    UNUSED(p); UNUSED(port); UNUSED(data);
    VXT_PRINT("writing unmapped IO port: %X\n", port);
}

static vxt_byte read(void *p, vxt_pointer addr) {
    UNUSED(p); UNUSED(addr);
    VXT_PRINT("reading unmapped memory: %X\n", addr);
    return 0xFF;
}

static void write(void *p, vxt_pointer addr, vxt_byte data) {
    UNUSED(p); UNUSED(addr); UNUSED(data);
    VXT_PRINT("writing unmapped memory: %X\n", addr);
}

static vxt_error destroy(void *p) {
    (void)p;
    // Prevent the use of freeing memory with default
    // allocator, by setting up a empty destroy function.
    return VXT_NO_ERROR;
}

static vxt_error install(void *p, vxt_system *s) {
    struct vxt_pirepheral *pi = VXT_GET_PIREPHERAL(p);
    vxt_system_install_io(s, pi, 0x0, 0xFFFF);
    vxt_system_install_mem(s, pi, 0x0, 0xFFFFF);
    return VXT_NO_ERROR;
}

void init_dummy_device(vxt_system *s) {
    struct _vxt_pirepheral *dummy = &((struct system*)s)->dummy;
    dummy->s = s;

    struct vxt_pirepheral *d = &dummy->p;

    d->install = &install;
    d->destroy = &destroy;
    d->io.in = &in;
    d->io.out = &out;
    d->io.read = &read;
    d->io.write = &write;
}
