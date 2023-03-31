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

#ifdef __linux__

#include <ch36x_lib.h>

VXT_PIREPHERAL(isa, {
    int fd;

    struct {
        const char *device;
        vxt_word io_start;
        vxt_word io_end;
        vxt_pointer mem_start;
        vxt_pointer mem_end;
    } config;
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    vxt_byte data;
    if (ch36x_read_io_byte((VXT_GET_DEVICE(isa, p))->fd, (unsigned long)port, &data))
        VXT_LOG("ERROR: Could not read IO port 0x%X!", port);
    return data;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    if (ch36x_write_io_byte((VXT_GET_DEVICE(isa, p))->fd, (unsigned long)port, data))
        VXT_LOG("ERROR: Could not write IO port 0x%X!", port);
}

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    vxt_byte data;
    if (ch36x_read_mem_byte((VXT_GET_DEVICE(isa, p))->fd, (unsigned long)addr, &data))
        VXT_LOG("ERROR: Could not read memory at 0x%X!", addr);
    return data;
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    if (ch36x_write_mem_byte((VXT_GET_DEVICE(isa, p))->fd, (unsigned long)addr, data))
        VXT_LOG("ERROR: Could not write memory at 0x%X!", addr);
}

static vxt_error setup_adapter(struct isa *d) {
    (void)d;
    // TODO
    return VXT_NO_ERROR;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, isa, p);
    vxt_system_install_io(s, p, d->config.io_start, d->config.io_end);
    if (d->config.mem_start || d->config.mem_end)
        vxt_system_install_mem(s, p, d->config.mem_start, d->config.mem_end);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, isa, p);
    ch36x_close(d->fd);
    return setup_adapter(d);
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    ch36x_close((VXT_GET_DEVICE(isa, p))->fd);
    vxt_system_allocator(VXT_GET_SYSTEM(isa, p))(p, 0);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "ISA Passthrough (CH367)";
}

struct vxt_pirepheral *vxtp_isa_create(vxt_allocator *alloc, const char *device, vxt_word io_start, vxt_word io_end, vxt_pointer mem_start, vxt_pointer mem_end) VXT_PIREPHERAL_CREATE(alloc, isa, {
    DEVICE->config.device = device;
    DEVICE->config.io_start = io_start;
    DEVICE->config.io_end = io_end;
    DEVICE->config.mem_start = mem_start;
    DEVICE->config.mem_end = mem_end;

    PIREPHERAL->install = &install;
    PIREPHERAL->destroy = &destroy;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
})

#else

struct vxt_pirepheral *vxtp_isa_create(vxt_allocator *alloc, const char *device, vxt_word io_start, vxt_word io_end, vxt_pointer mem_start, vxt_pointer mem_end) {
    (void)alloc; (void)device; (void)io_start; (void)io_end; (void)mem_start; (void)mem_end;
    return NULL;
}

#endif
