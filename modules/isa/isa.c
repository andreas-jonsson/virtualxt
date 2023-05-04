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

#ifdef __linux__

#include "ch36x/ch36x_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VXT_PIREPHERAL(isa, {
    int fd;
    unsigned long io_base;
    unsigned long mem_base;

    struct {
        char device[32];
        int irq;
        vxt_word io_start;
        vxt_word io_end;
        vxt_pointer mem_start;
        vxt_pointer mem_end;
    } config;
})

int interrupt_trigger = 0;

static void ch36x_isr_handler(int signo) {
    (void)signo;
    interrupt_trigger++;
}

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    VXT_DEC_DEVICE(d, isa, p);
    vxt_byte data;
    if (ch36x_read_io_byte(d->fd, d->io_base + (unsigned long)port, &data))
        VXT_LOG("ERROR: Could not read IO port 0x%X!", port);
    return data;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(d, isa, p);
    if (ch36x_write_io_byte(d->fd, d->io_base + (unsigned long)port, data))
        VXT_LOG("ERROR: Could not write IO port 0x%X!", port);
}

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(d, isa, p);
    vxt_byte data;
    if (ch36x_read_mem_byte(d->fd, d->mem_base + (unsigned long)addr, &data))
        VXT_LOG("ERROR: Could not read memory at 0x%X!", addr);
    return data;
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(d, isa, p);
    if (ch36x_write_mem_byte(d->fd, d->mem_base + (unsigned long)addr, data))
        VXT_LOG("ERROR: Could not write memory at 0x%X!", addr);
}

static vxt_error setup_adapter(struct isa *d) {
    d->fd = ch36x_open(d->config.device);
    if (d->fd < 0) {
        VXT_LOG("ERROR: Could not open: %s", d->config.device);
        return VXT_USER_ERROR(0);
    }

    enum CHIP_TYPE ct;
    if (ch36x_get_chiptype(d->fd, &ct)) {
        VXT_LOG("ERROR: Could not read chip type!");
        return VXT_USER_ERROR(1);
    }

    switch (ct) {
        case CHIP_CH365:
            VXT_LOG("ISA Passthrugh device: CH365");
            break;
        case CHIP_CH367:
            VXT_LOG("ISA Passthrugh device: CH367");
            break;
        case CHIP_CH368:
            VXT_LOG("ISA Passthrugh device: CH368");
            break;
        default:
            VXT_LOG("ERROR: Unsupported chip type!");
            return VXT_USER_ERROR(2);
    }

    if (ch36x_get_ioaddr(d->fd, &d->io_base)) {
        VXT_LOG("ERROR: Could not get IO base address!");
        return VXT_USER_ERROR(3);
    }
    VXT_LOG("IO base address: 0x%lX", d->io_base);

    if ((ct == CHIP_CH368) && ch36x_get_memaddr(d->fd, &d->mem_base)) {
        VXT_LOG("ERROR: Could not get memory base address!");
        return VXT_USER_ERROR(4);
    }
    VXT_LOG("Memory base address: 0x%lX", d->mem_base);

    if (d->config.irq >= 0) {
        if (ch36x_enable_isr(d->fd, INT_FALLING)) {
            VXT_LOG("ERROR: Could not enable ISR!");
            return VXT_USER_ERROR(5);
        }
        ch36x_set_int_routine(d->fd, &ch36x_isr_handler);
        VXT_LOG("Passthrough interrupt: %d", d->config.irq);
    }
    return VXT_NO_ERROR;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, isa, p);
    if (d->config.io_start || d->config.io_end)
        vxt_system_install_io(s, p, d->config.io_start, d->config.io_end);
    if (d->config.mem_start || d->config.mem_end)
        vxt_system_install_mem(s, p, d->config.mem_start, d->config.mem_end);
    if (d->config.irq >= 0)
        vxt_system_install_timer(s, p, 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, isa, p);
    if (d->fd) {
        ch36x_set_int_routine(d->fd, NULL);
        ch36x_close(d->fd);
        d->fd = -1;
    }
    return setup_adapter(d);
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(d, isa, p);
    if (d->fd) {
        ch36x_set_int_routine(d->fd, NULL);
        ch36x_close(d->fd);
        d->fd = -1;
    }
    vxt_system_allocator(VXT_GET_SYSTEM(isa, p))(p, 0);
    return VXT_NO_ERROR;
}

static vxt_error timer(struct vxt_pirepheral *p, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    VXT_DEC_DEVICE(d, isa, p);
    if (interrupt_trigger) {
        interrupt_trigger = 0;
        vxt_system_interrupt(VXT_GET_SYSTEM(isa, p), d->config.irq);
    }
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "ISA Passthrough (CH367)";
}

static vxt_error config(struct vxt_pirepheral *p, const char *section, const char *key, const char *value) {
    VXT_DEC_DEVICE(d, isa, p);
    if (!strcmp("ch36x", section)) {
        if (!strcmp("device", key)) {
            strncpy(d->config.device, value, sizeof(d->config.device) - 1);
        } else if (!strcmp("memory", key)) {
            if (sscanf(value, "%x,%x", &d->config.mem_start, &d->config.mem_end) < 2)
                d->config.mem_end = d->config.mem_start + 1;
        } else if (!strcmp("io", key)) {
            if (sscanf(value, "%hx,%hx", &d->config.io_start, &d->config.io_end) < 2)
                d->config.io_end = d->config.io_start + 1;
        } else if (!strcmp("irq", key)) {
            d->config.irq = atoi(value);
        }
    }
    return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(isa, {
    DEVICE->fd = -1;
    DEVICE->config.irq = -1;
    strcpy(DEVICE->config.device, "/dev/ch36xpci0");

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

#else

VXT_PIREPHERAL(isa, { int _; })
VXTU_MODULE_CREATE(isa, {})

#endif
