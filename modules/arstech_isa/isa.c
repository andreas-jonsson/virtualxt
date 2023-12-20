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

#include <vxt/vxtu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// From libarsusb4
extern unsigned char in8(unsigned short port);
extern void out8(unsigned short port, unsigned char data);
extern unsigned char rd8(unsigned long phadr);
extern void wr8(unsigned long phadr, unsigned char data);
extern unsigned long GetIrqDma(void);
extern int ArsInit(void);
extern void ArsExit(void);

struct arstech_isa {
    struct {
        vxt_word io_start;
        vxt_word io_end;
        vxt_pointer mem_start;
        vxt_pointer mem_end;
        int irq_poll;
    } config;
};

static vxt_byte in(struct arstech_isa *d, vxt_word port) {
	(void)d;
    return in8(port);
}

static void out(struct arstech_isa *d, vxt_word port, vxt_byte data) {
	(void)d;
    out8(port, data);
}

static vxt_byte read(struct arstech_isa *d, vxt_pointer addr) {
	(void)d;
    return rd8(addr);
}

static void write(struct arstech_isa *d, vxt_pointer addr, vxt_byte data) {
	(void)d;
    wr8(addr, data);
}

static vxt_error install(struct arstech_isa *d, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(d);
    if (d->config.io_start || d->config.io_end)
        vxt_system_install_io(s, p, d->config.io_start, d->config.io_end);
    if (d->config.mem_start || d->config.mem_end)
        vxt_system_install_mem(s, p, d->config.mem_start, d->config.mem_end);
    if (d->config.irq_poll >= 0)
        vxt_system_install_timer(s, p, (unsigned int)d->config.irq_poll);

    if (!ArsInit()) {
        VXT_LOG("ERROR: Could not initialize Arstech USB library!");
        return VXT_USER_ERROR(0);
    }
    return VXT_NO_ERROR;
}

static vxt_error reset(struct arstech_isa *d) {
	(void)d;
    ArsExit();
    return ArsInit() ? VXT_NO_ERROR : VXT_USER_ERROR(0);
}

static vxt_error destroy(struct arstech_isa *d) {
    ArsExit();
    vxt_system_allocator(VXT_GET_SYSTEM(d))(VXT_GET_PIREPHERAL(d), 0);
    return VXT_NO_ERROR;
}

static vxt_error timer(struct arstech_isa *d, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    unsigned long irq = GetIrqDma();
    for (int i = 0; i < 8; i++) {
        if (irq & (1 << i))
            vxt_system_interrupt(VXT_GET_SYSTEM(d), i);
    }
    return VXT_NO_ERROR;
}

static const char *name(struct arstech_isa *d) {
    (void)d; return "ISA Passthrough (Arstech USB)";
}

static vxt_error config(struct arstech_isa *d, const char *section, const char *key, const char *value) {
    if (!strcmp("arstech_isa", section)) {
        if (!strcmp("memory", key)) {
            if (sscanf(value, "%x,%x", &d->config.mem_start, &d->config.mem_end) < 2)
                d->config.mem_end = d->config.mem_start + 1;
        } else if (!strcmp("port", key)) {
            if (sscanf(value, "%hx,%hx", &d->config.io_start, &d->config.io_end) < 2)
                d->config.io_end = d->config.io_start + 1;
        } else if (!strcmp("irq-poll", key)) {
            d->config.irq_poll = atoi(value);
        }
    }
    return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(arstech_isa, {
    DEVICE->config.irq_poll = -1;

    PIREPHERAL->install = &install;
    PIREPHERAL->destroy = &destroy;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->config = &config;
    PIREPHERAL->timer = &timer;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
})
