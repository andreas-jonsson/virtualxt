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

VXT_PIREPHERAL(debugger, {
    const char *trace;
    vxt_pointer cursor;
    vxt_pointer breakpoint;
    vxt_pointer until;
    vxt_pointer watch;

    bool (*pdisasm)(vxt_system*, const char*, vxt_pointer, int, int);
    const char *(*getline)(void);
    int (*print)(const char*, ...);

	vxt_byte io_map[VXT_IO_MAP_SIZE];
	vxt_byte mem_map[VXT_MEM_MAP_SIZE];
})

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

static bool strcomp(const char *a, const char *b) {
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
        "  h      Displays this text.\n"
        "  q      Shutdown emulator.\n"
        "  c      Continue execution.\n"
        "  s      Step to next instruction.\n"
        "  sb     Set breakpoint.\n"
        "  cb     Clear breakpoint.\n"
        "  sw     Set memory watch.\n"
        "  cw     Clear memory watch.\n"
        "  n(X)   Display disassembly of address X.\n"
        "  m(X)   Show memory dump of data at address X.\n"
        "  ?      Disassemble next instruction.\n"
        "  @X     Read byte at address X.\n"
        "  @@X    Read word at address X.\n"
        "  #(X)   Print X number of stack entries.\n"
        "  uX     Continue execution until CS:IP == CS:X.\n"
        "  f      Display CPU flags.\n"
        "  R(.)   Display content of register R.\n"
    );
}

static vxt_error read_command(vxt_system *s, struct debugger * const dbg) {
    #define CMD(str) else if (strcomp(line, (str)))
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

    #define DISASM(c, z, l)                                                                         \
        if (dbg->pdisasm && !dbg->pdisasm(s, NULL, (c), (z), (l)))                                  \
            dbg->print("Unable to disassemble code! Is ndisasm installed?\n");                      \

    CONSTSP(vxt_registers) regs = vxt_system_registers(s);
    for (;;) {
        dbg->print("%0*X:%0*X>", 4, regs->cs, 4, regs->ip);
        const char *line = dbg->getline();
        ENSURE(line);

        if (strcomp(line, "h")) {
            print_help(dbg);
        } CMD("c") {
            regs->debug = false;
            return VXT_NO_ERROR;
        } CMD("q") {
            return VXT_USER_TERMINATION;
        } CMD("s") {
            DISASM(VXT_POINTER(regs->cs, regs->ip), 16, 1);
            return VXT_NO_ERROR;
        } CMD("f") {
            #define FLAG_SYM(f, c) dbg->print("%c", (regs->flags & f) ? c : '-')
            dbg->print("????");
            FLAG_SYM(VXT_OVERFLOW, 'O');
            FLAG_SYM(VXT_DIRECTION, 'D');
            FLAG_SYM(VXT_INTERRUPT, 'I');
            FLAG_SYM(VXT_TRAP, 'T');
            FLAG_SYM(VXT_SIGN, 'S');
            FLAG_SYM(VXT_ZERO, 'Z');
            dbg->print("?");
            FLAG_SYM(VXT_AUXILIARY, 'A');
            dbg->print("?");
            FLAG_SYM(VXT_PARITY, 'P');
            dbg->print("?");
            FLAG_SYM(VXT_CARRY, 'C');
            #undef FLAG_SYM
            dbg->print(" (0x%0*X)\n", 4, regs->flags);
        } CMD("sb") {
            dbg->print("breakpoint>");
            const char *ln = dbg->getline();
            ENSURE(ln);
            dbg->breakpoint = (vxt_pointer)tonumber(ln) & 0xFFFFF;
        } CMD("cb") {
            dbg->breakpoint = -1;
        } CMD("sw") {
            dbg->print("watch>");
            const char *ln = dbg->getline();
            ENSURE(ln);
            dbg->watch = (vxt_pointer)tonumber(ln) & 0xFFFFF;
        } CMD("cw") {
            dbg->watch = -1;
        } CMD("?") {
            DISASM(VXT_POINTER(regs->cs, regs->ip), 16, 1);
        } PREFIX("n") {
            if (dbg->pdisasm) {
                dbg->cursor = (line[1] ? tonumber(&line[1]) : dbg->cursor + 256);
                DISASM(dbg->cursor, 256, 20);
            }
        } PREFIX("m") {
            dbg->cursor = (line[1] ? tonumber(&line[1]) : dbg->cursor + 256) & 0xFFFF0;
            for (int i = 0; i < 16; i++) {
                dbg->print("%0*X ", 5, (dbg->cursor + i * 16) & 0xFFFFF);
                for (int j = 0; j < 16; j++) {
                    if (j % 4 == 0)
                        dbg->print(" ");
                    dbg->print("%0*X ", 2, vxt_system_read_byte(s, dbg->cursor + (i * 16) + j));
                }
                dbg->print(" ");
                for (int j = 0; j < 16; j++)
                    dbg->print("%c", toprint(vxt_system_read_byte(s, dbg->cursor + (i * 16) + j)));
                dbg->print("\n");
            }
        } PREFIX("u") {
            dbg->until = VXT_POINTER(regs->cs, tonumber(&line[1]));
            regs->debug = false;
            return VXT_NO_ERROR;
        } PREFIX("@@") {
            vxt_word n = vxt_system_read_word(s, (vxt_pointer)tonumber(&line[2]));
            dbg->print("0x%X (%d, '%c%c')\n", n, n, toprint((vxt_byte)(n>>8)), toprint((vxt_byte)(n&0xF)));
        } PREFIX("@") {
            vxt_byte n = vxt_system_read_byte(s, (vxt_pointer)tonumber(&line[1]));
            dbg->print("0x%X (%d, '%c')\n", n, n, toprint(n));
        } PREFIX("#") {
            unsigned c = tonumber(&line[1]);
            c = c ? c : 1;
            for (unsigned i = 0; i < c; i++) {
                vxt_word offset = regs->sp + (vxt_word)i * 2;
                vxt_word n = vxt_system_read_word(s, VXT_POINTER(regs->ss, offset));
                dbg->print("%0*X: 0x%0*X (%d, '%c%c')\n", 4, offset, 4, n, n, toprint((vxt_byte)(n>>8)), toprint((vxt_byte)(n&0xF)));
            }
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

static vxt_byte read(struct vxt_pirepheral *p, vxt_pointer addr) {
    VXT_DEC_DEVICE(dbg, debugger, p);
    CONSTSP(vxt_pirepheral) dev = vxt_system_pirepheral(vxt_pirepheral_system(p), dbg->mem_map[addr]);
    return dev->io.read(dev, addr);
}

static void write(struct vxt_pirepheral *p, vxt_pointer addr, vxt_byte data) {
    VXT_DEC_DEVICE(dbg, debugger, p);
    vxt_system *s = vxt_pirepheral_system(p);

    if (dbg->watch == addr) {
        dbg->print("Memory watch triggered: 0x%X <- 0x%X (%d)\n", addr, data, data);
        vxt_system_registers(s)->debug = true;
    }

    CONSTSP(vxt_pirepheral) dev = vxt_system_pirepheral(s, dbg->mem_map[addr]);
    dev->io.write(dev, addr, data);
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(dbg, debugger, p);

    if (dbg->trace && dbg->pdisasm)
        dbg->pdisasm(s, dbg->trace, 0, 0, 0);

    const vxt_byte *io = vxt_system_io_map(s);
    for (int i = 0; i < VXT_IO_MAP_SIZE; i++)
        dbg->io_map[i] = io[i];

    const vxt_byte *mem = vxt_system_mem_map(s);
    for (int i = 0; i < VXT_MEM_MAP_SIZE; i++)
        dbg->mem_map[i] = mem[i];

    vxt_system_install_mem(s, p, 0, 0xFFFFF);
    return VXT_NO_ERROR;
}

static vxt_error step(struct vxt_pirepheral *p, int cycles) {
    VXT_DEC_DEVICE(dbg, debugger, p); UNUSED(cycles);
    vxt_system *s = vxt_pirepheral_system(p);
    CONSTSP(vxt_registers) regs = vxt_system_registers(s);

    if (dbg->breakpoint == VXT_POINTER(regs->cs, regs->ip)) {
        dbg->print("Breakpoint triggered!\n");
        regs->debug = true;
    }

    if (dbg->until <= VXT_POINTER(regs->cs, regs->ip)) {
        dbg->until = -1;
        regs->debug = true;
    }

    vxt_error err = VXT_NO_ERROR;
    if (regs->debug)
        err = read_command(s, dbg);

    if (dbg->trace && dbg->pdisasm) {
        if (!dbg->pdisasm(s, dbg->trace, VXT_POINTER(regs->cs, regs->ip), 16, 1)) {
            LOG("Could not write trace!");
            dbg->trace = NULL;
        }
    }
    return err;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(vxt_pirepheral_system(p))(p, 0);
    return VXT_NO_ERROR;
}

static enum vxt_pclass pclass(struct vxt_pirepheral *p) {
    UNUSED(p); return VXT_PCLASS_DEBUGGER;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p; return "Debugger";
}

void vxtu_debugger_interrupt(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(dbg, debugger, p);
    vxt_system_registers(vxt_pirepheral_system(p))->debug = true;
    dbg->print("\aInterrupted!\n");
}

struct vxt_pirepheral *vxtu_create_debugger(vxt_allocator *alloc, const struct vxtu_debugger_interface *interface) {
    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(debugger));
    memclear(p, VXT_PIREPHERAL_SIZE(debugger));
    VXT_DEC_DEVICE(d, debugger, p);

    d->trace = interface->trace;
    d->pdisasm = interface->pdisasm;
    d->getline = interface->getline;
    d->print = interface->print;
    d->breakpoint = d->until = d->watch = -1;

    p->install = &install;
    p->destroy = &destroy;
    p->step = &step;
    p->pclass = &pclass;
    p->name = &name;
    p->io.read = &read;
    p->io.write = &write;
    return p;
}
