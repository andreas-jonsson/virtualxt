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
    vxt_pointer cursor;

    bool (*pdisasm)(vxt_system*, vxt_pointer, int);
    const char *(*getline)(void);
    int (*print)(const char*, ...);

	vxt_byte io_map[VXT_IO_MAP_SIZE];
	vxt_byte mem_map[VXT_MEM_MAP_SIZE];
};

static bool valid_char(char ch, unsigned base) {
    if (!ch)
        return false;
    const char *base_str = "0123456789";
    if (base == 2)
        base_str = "01";
    else if (base == 16)
        base_str = "0123456789abcdefABCDEF";

    do {
        if (*base_str == ch)
            return true;
    } while (++base_str);
    return false;
}

static char toprint(vxt_byte v) {
    if (v >= 32 && v <= 125)
        return (char)v;
    return '.';
}

static unsigned tobin(char ch) {
    if (ch >= 'a' && ch <= 'f')
        return (ch - 'a') + 10;
    if (ch >= 'A' && ch <= 'F')
        return (ch - 'A') + 10;
  return ch - '0';
}

static unsigned tonumber(const char *str) {
    unsigned res = 0;
    unsigned base = 10;
    if (str[0] == '0') {
        if (str[1] == 'x') {
            base = 16;
            str = &str[2];
        } else if (str[1] == 'b') {
            base = 2;
            str = &str[2];
        }
    }
    for (int i = 0; valid_char(str[i], base); i++)
        res = res * base + tobin(str[i]);
    return res;
}

static bool strcmp(const char *a, const char *b) {
    do {
        if (*a != *b)
            return false;
        a++; b++;
    } while (*a && *b);
    return *a == *b;
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
    dbg->print(
        "  h      Displays this text\n"
        "  q      Shutdown emulator\n"
        "  c      Continue execution\n"
        "  s      Step to next instruction\n"
        "  d(X)   Display disassembly of address X\n"
        "  m(X)   Show memory dump of data at address X\n"
        "  ?      Display disassembly near IP\n"
        "  @X     Read byte at address X\n"
        "  @@X    Read word at address X\n"
        "  f      Display CPU flags\n"
        "  R(.)   Display content of register R\n"
    );
}

static vxt_error read_command(struct debugger * const dbg) {
    #define CMD(str) else if (strcmp(line, (str)))
    #define PREFIX(str) else if (has_prefix(line, (str)))

    #define REG1(r) PREFIX(#r) {                    \
            if (line[2] == '.')                     \
                dbg->print("%d\n", regs->r);        \
            else                                    \
                dbg->print("0x%X\n", regs->r);      \
        }                                           \

    #define REG(r)          \
        REG1(r ## l)        \
        REG1(r ## h)        \
        REG1(r ## x)        \

    #define DISASM(c, z)                                        \
        if (dbg->pdisasm && !dbg->pdisasm(dbg->s, (c), (z)))    \
            dbg->print("Unable to disassemble code!\n");        \

    CONSTSP(vxt_registers) regs = vxt_system_registers(dbg->s);
    for (;;) {
        dbg->print("%0*X:%0*X>", 4, regs->cs, 4, regs->ip);
        const char *line = dbg->getline();
        ENSURE(line);

        if (strcmp(line, "h")) {
            print_help(dbg);
        } CMD("c") {
            dbg->halt = false;
            return VXT_NO_ERROR;
        } CMD("q") {
            return VXT_USER_TERMINATION;
        } CMD("s") {
            return VXT_NO_ERROR;
        } CMD("f") {
            dbg->print("%0*X\n", 4, regs->flags);
        } CMD("?") {
            DISASM(POINTER(regs->cs, regs->ip), 8);
        } PREFIX("d") {
            if (dbg->pdisasm) {
                dbg->cursor = (line[1] ? tonumber(&line[1]) : dbg->cursor + 64);
                DISASM(dbg->cursor, 64);
            }
        } PREFIX("m") {
            dbg->cursor = (line[1] ? tonumber(&line[1]) : dbg->cursor + 256) & 0xFFFF0;
            for (int i = 0; i < 16; i++) {
                dbg->print("%0*X ", 5, (dbg->cursor + i * 16) & 0xFFFFF);
                for (int j = 0; j < 16; j++) {
                    if (j % 4 == 0)
                        dbg->print(" ");
                    dbg->print("%0*X ", 2, vxt_system_read_byte(dbg->s, dbg->cursor + (i * 16) + j));
                }
                dbg->print(" ");
                for (int j = 0; j < 16; j++)
                    dbg->print("%c", toprint(vxt_system_read_byte(dbg->s, dbg->cursor + (i * 16) + j)));
                dbg->print("\n");
            }
        } PREFIX("@@") {
            vxt_word n = vxt_system_read_word(dbg->s, (vxt_pointer)tonumber(&line[2]));
            dbg->print("0x%X (%d, '%c%c')\n", n, n, toprint((vxt_byte)(n>>8)), toprint((vxt_byte)(n&0xF)));
        } PREFIX("@") {
            vxt_byte n = vxt_system_read_byte(dbg->s, (vxt_pointer)tonumber(&line[1]));
            dbg->print("0x%X (%d, '%c')\n", n, n, toprint(n));
        }
        REG(a) REG(b) REG(c) REG(d)
        REG1(cs) REG1(ss) REG1(ds) REG1(es)
        REG1(sp) REG1(bp) REG1(si) REG1(di) REG1(ip)
        else {
            dbg->print("Unknown command! Type 'h' for help.\n");
        }
    }

    #undef DISASM
    #undef REG
    #undef REG1
    #undef CMD
    #undef PREFIX

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
    DEC_DEVICE(dbg, debugger, d->userdata);
    dbg->halt = true;
    dbg->print("\aInterrupted!\n");
}

struct vxt_pirepheral vxtu_create_debugger_device(vxt_allocator *alloc, const struct vxtu_debugger_interface *interface) {
    struct debugger *d = (struct debugger*)alloc(NULL, sizeof(struct debugger));
    memclear(d, sizeof(struct debugger));

    d->halt = interface->halt;
    d->pdisasm = interface->pdisasm;
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
