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

// Reference: https://www.lo-tech.co.uk/wiki/Lo-tech_2MB_EMS_Board

#include <vxt/vxtu.h>

#include <stdio.h>
#include <string.h>

// The Lo-tech EMS board driver is hardcoded to 2MB.
#define MEMORY_SIZE 0x200000

VXT_PIREPHERAL(ems, {
 	vxt_byte mem[MEMORY_SIZE];
    vxt_pointer mem_base;
    vxt_word io_base;
    vxt_byte page_selectors[4];
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    (void)p; (void)port;
    VXT_LOG("Register read is not supported!");
    return 0xFF;
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
    VXT_DEC_DEVICE(m, ems, p);
    int sel = port - m->io_base;
    m->page_selectors[sel & 3] = data;
}

static vxt_pointer physical_address(struct ems *m, vxt_pointer addr) {
    vxt_pointer frame_addr = addr - m->mem_base;
    vxt_pointer page_addr = frame_addr & 0x3FFF;
    vxt_byte selector = m->page_selectors[(frame_addr >> 14) & 3];
    return selector * 0x4000 + page_addr;
}

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(m, ems, p);
    vxt_pointer phys_addr = physical_address(m, addr);
    return (phys_addr < MEMORY_SIZE) ? m->mem[phys_addr] : 0xFF;
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(m, ems, p);
    vxt_pointer phys_addr = physical_address(m, addr);
    if (phys_addr < MEMORY_SIZE)
        m->mem[phys_addr] = data;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(m, ems, p);
    vxt_system_install_io(s, p, m->io_base, m->io_base + 3);
    vxt_system_install_mem(s, p, m->mem_base, m->mem_base + 0xFFFF);
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Lo-tech 2MB EMS Board";
}

static vxt_error config(struct vxt_pirepheral *p, const char *section, const char *key, const char *value) {
    VXT_DEC_DEVICE(m, ems, p);
    if (!strcmp("lotech_ems", section)) {
        if (!strcmp("memory", key)) sscanf(value, "%x", &m->mem_base);
        else if (!strcmp("port", key)) sscanf(value, "%hx", &m->io_base);
    }
    return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(ems, {
    if (strcmp(ARGS, "lotech_ems"))
        return NULL;
    
    vxtu_randomize(DEVICE->mem, MEMORY_SIZE, (intptr_t)PIREPHERAL);

    DEVICE->mem_base = 0xE0000;
    DEVICE->io_base = 0x260;

    PIREPHERAL->install = &install;
    PIREPHERAL->config = &config;
    PIREPHERAL->name = &name;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})
