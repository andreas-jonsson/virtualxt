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

#include "vxtp.h"

VXT_PIREPHERAL(serial_dbg, {
    vxt_word base_port;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    (void)p; (void)port;
    VXT_DEC_DEVICE(d, serial_dbg, p);
    vxt_word reg = port - d->base_port;
    if (reg == 5)
        return 0x20; // Set transmission holding register empty (THRE); data can be sent.
    return 0;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    (void)p; (void)port;
    VXT_PRINT("%c", data);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, serial_dbg, p);
    vxt_system_install_io(s, p, d->base_port, d->base_port + 7);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(serial_dbg, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Serial Debug Printer";
}

struct vxt_pirepheral *vxtp_serial_dbg_create(vxt_allocator *alloc, vxt_word base_port) VXT_PIREPHERAL_CREATE(alloc, serial_dbg, {
    DEVICE->base_port = base_port;

    PIREPHERAL->install = &install;
    PIREPHERAL->destroy = &destroy;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
