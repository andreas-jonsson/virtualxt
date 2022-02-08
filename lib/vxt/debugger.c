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

#include <vxt/utils.h>
#include "common.h"

struct debugger {
	vxt_system *s;
    bool halt;

    const char *(*getline)(void);
    int (*print)(const char*, ...);

	vxt_byte io_map[VXT_IO_MAP_SIZE];
	vxt_byte mem_map[VXT_MEM_MAP_SIZE];
};

static unsigned tonumber(const char *str) {
    unsigned res = 0;
    unsigned base = 10;
    if (str[0] == '0') {
        if (str[1] == 'x')
            base = 16;
        else if (str[1] == 'b')
            base = 2;
    }
    for (int i = 0; str[i]; i++)
        res = res * base + str[i] - '0';
    return res;
}

static bool has_prefix(const char *a, const char *b) {
    do {
        if (*a != *b)
            return false;
        a++; b++;
    } while (*a && *b);
    return true;
}

static void print_help(struct debugger * const dbg) {
    dbg->print("TODO...\n");
}

static vxt_error read_command(struct debugger * const dbg) {
    #define CMD(str) else if (has_prefix(line, (str)))

    #define REG1(r) CMD(#r) {                   \
            if (line[2] == '.')                 \
                dbg->print("%d\n", regs->r);    \
            else                                \
                dbg->print("0x%X\n", regs->r);  \
        }                                       \

    #define REG(r)                              \
        REG1(r ## l)                            \
        REG1(r ## h)                            \
        REG1(r ## x)                            \

    CONSTSP(vxt_registers) regs = vxt_system_registers(dbg->s);
    for (;;) {
        dbg->print("%0*X:%0*X>", 4, regs->cs, 4, regs->ip);
        const char *line = dbg->getline();
        ENSURE(line);

        if (has_prefix(line, "h")) {
            print_help(dbg);
        } REG(a) REG(b) REG(c) REG(d)
        REG1(cs) REG1(ss) REG1(ds) REG1(es)
        REG1(sp) REG1(bp) REG1(si) REG1(di) REG1(ip)
        CMD("@") {
            unsigned addr = tonumber(&line[1]);
            dbg->print("0x%X\n", vxt_system_read_byte(dbg->s, (vxt_pointer)addr));
        } CMD("c") {
            dbg->halt = false;
            return VXT_NO_ERROR;
        } CMD("q") {
            return VXT_USER_TERMINATION;
        } CMD("s") {
            return VXT_NO_ERROR;
        } else {
            dbg->print("Unknown command!\n");
        }
    }

    #undef REG
    #undef REG1
    #undef CMD

    UNREACHABLE(0);
}

static vxt_byte read(void *d, vxt_pointer addr) {
    DEC_DEVICE(dbg, debugger, d);
    const CONSTSP(vxt_pirepheral) dev = vxt_system_pirepheral(dbg->s, dbg->mem_map[addr]);
    return dev->io.read(dev->userdata, addr);
}

static void write(void *d, vxt_pointer addr, vxt_byte data) {
    DEC_DEVICE(dbg, debugger, d);
    const CONSTSP(vxt_pirepheral) dev = vxt_system_pirepheral(dbg->s, dbg->mem_map[addr]);
    dev->io.write(dev->userdata, addr, data);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *d) {
    DEC_DEVICE(dbg, debugger, d->userdata);
    dbg->s = s;

    const vxt_byte *io = vxt_system_io_map(s);
    for (int i = 0; i < VXT_IO_MAP_SIZE; i++)
        dbg->io_map[i] = io[i];

    const vxt_byte *mem = vxt_system_mem_map(s);
    for (int i = 0; i < VXT_MEM_MAP_SIZE; i++)
        dbg->mem_map[i] = mem[i];

    vxt_system_install_mem(s, d, 0, 0xFFFFF);
    return VXT_NO_ERROR;
}

static vxt_error step(void *d, int cycles) {
    DEC_DEVICE(dbg, debugger, d); (void)cycles;
    if (dbg->halt)
        return read_command(dbg);
    return VXT_NO_ERROR;
}

static vxt_error destroy(void *d) {
    DEC_DEVICE(dbg, debugger, d);
    vxt_system_allocator(dbg->s)(d, 0);
    return VXT_NO_ERROR;
}

void vxtu_debugger_interrupt(struct vxt_pirepheral *d) {
    DEC_DEVICE(dbg, debugger, d);
    dbg->halt = true;
}

struct vxt_pirepheral vxtu_create_debugger_device(vxt_allocator *alloc, const struct vxtu_debugger_interface *interface) {
    struct debugger *d = (struct debugger*)alloc(NULL, sizeof(struct debugger));
    memclear(d, sizeof(struct debugger));

    d->halt = interface->halt;
    d->getline = interface->getline;
    d->print = interface->print;

    struct vxt_pirepheral p = {0};
    p.userdata = d;
    p.install = &install;
    p.destroy = &destroy;
    p.step = &step;
    p.io.read = &read;
    p.io.write = &write;
    return p;
}
