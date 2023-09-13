#!/bin/env python

import gzip
import json
import os
import requests
import struct

c_header = """// This file is generated!

#include <vxt/vxtu.h>
#include "testing.h"

VXT_PACK(struct) registers {
    vxt_word ax, bx, cx, dx;
    vxt_word cs, ss, ds, es;
    vxt_word sp, bp, si, di;
    vxt_word ip, flags;
};
                                                     
VXT_PACK(struct) memory {
    vxt_dword addr;
    vxt_byte data;
};

#define NAME_SIZE 256
#define CHECK_REG(reg) TASSERT(r->reg == regs.reg, "Expected the value of register '" #reg "' to be 0x%X(%d) but it was 0x%X(%d)", regs.reg, regs.reg, r->reg, r->reg)

static int execute_test(struct Test T, char *name, const char *input) {
    struct vxt_pirepheral *devices[] = {
        vxtu_memory_create(&TALLOC, 0x0, 0x100000, false),
        NULL
    };

    vxt_system *s = vxt_system_create(&TALLOC, VXT_CPU_8088, VXT_DEFAULT_FREQUENCY, devices);
    TENSURE(s);
    TENSURE_NO_ERR(vxt_system_initialize(s));

    FILE *fp = fopen(input, "rb");
    TENSURE(fp);
    
    int num_tests;
    TENSURE(fread(&num_tests, 4, 1, fp) == 1);
    
    for (int i = 0; i < num_tests; i++) {
        vxt_word flags_mask;
        TENSURE(fread(&flags_mask, 2, 1, fp) == 1);

        for (int i = 0; (name[i] = (char)fgetc(fp)) && (i < (NAME_SIZE - 1)); i++);

        vxt_system_reset(s);
        struct vxt_registers *r = vxt_system_registers(s);

        struct registers regs;              
        TENSURE(fread(&regs, sizeof(struct registers), 1, fp) == 1);
        r->ax = regs.ax; r->bx = regs.bx; r->cx = regs.cx; r->dx = regs.dx;
        r->cs = regs.cs; r->ss = regs.ss; r->ds = regs.ds; r->es = regs.es;
        r->sp = regs.sp; r->bp = regs.bp; r->si = regs.si; r->di = regs.di;
        r->ip = regs.ip; r->flags = regs.flags;

        vxt_word num_mem;
        TENSURE(fread(&num_mem, 2, 1, fp) == 1);
       
        for (int i = 0; i < (int)num_mem; i++) {
            struct memory mem;
            TENSURE(fread(&mem, sizeof(struct memory), 1, fp) == 1);
            vxt_system_write_byte(s, mem.addr, mem.data);
        }
                                                        
        struct vxt_step step = vxt_system_step(s, 0);
        TENSURE_NO_ERR(step.err);

        TENSURE(fread(&regs, sizeof(struct registers), 1, fp) == 1);

        CHECK_REG(ax);
        CHECK_REG(bx);
        CHECK_REG(cx);
        CHECK_REG(dx);

        CHECK_REG(cs);
        CHECK_REG(ss);
        CHECK_REG(ds);
        CHECK_REG(es);

        CHECK_REG(sp);
        CHECK_REG(bp);
        CHECK_REG(si);
        CHECK_REG(di);

        CHECK_REG(ip);
        TASSERT((r->flags & flags_mask) == (regs.flags & flags_mask), "Expected flags register to be 0x%X but it was 0x%X", (regs.flags & flags_mask), (r->flags & flags_mask));

        TENSURE(fread(&num_mem, 2, 1, fp) == 1);
       
        for (int i = 0; i < (int)num_mem; i++) {
            struct memory mem;
            TENSURE(fread(&mem, sizeof(struct memory), 1, fp) == 1);
            vxt_byte data = vxt_system_read_byte(s, mem.addr);
            TASSERT(data == mem.data, "Expected memory at address 0x%X to be 0x%X(%d) but it is 0x%X(%d)", mem.addr, mem.data, mem.data, data, data);
        }
        
        // Just skip this for now.          
        TENSURE(fread(&num_mem, 2, 1, fp) == 1);
        TENSURE(fread(&num_mem, 1, 1, fp) == 1);
    }

    vxt_system_destroy(s);
    fclose(fp);
    return 0;
}
"""

c_body = """
TEST({name},
    char name[NAME_SIZE] = {{0}};
    if (execute_test(T, name, "{input}") != 0) {{
        TLOG("Execute: %s", name);
        return -1;
    }}
)
"""

c_test_name = "lib/vxt/i8088_tests.c"
mask_file = json.loads(open("tools/tests/8088v1/8088.json", "r").read())

def pack_test(data, output):
    regs = data["regs"]
    output.write(struct.pack("HHHHHHHHHHHHHH",
        regs["ax"], regs["bx"], regs["cx"], regs["dx"],
        regs["cs"], regs["ss"], regs["ds"], regs["es"],
        regs["sp"], regs["bp"], regs["si"], regs["di"],
        regs["ip"], regs["flags"]
    ))
    
    ram = data["ram"]
    output.write(struct.pack("H", len(ram)))
    for mem in ram:
        output.write(struct.pack("IB", mem[0], mem[1]))

def write_test(data, output):
    output.write(data["name"].encode("ASCII"))
    output.write(struct.pack("B", 0))
    pack_test(data["initial"], output)
    pack_test(data["final"], output)
    output.write(struct.pack("HB", len(data["cycles"]), len(data["final"]["queue"])))

def check_and_download(filename):
    filepath = "tools/tests/8088v1/" + filename
    if not os.path.exists(filepath):
        print("Downloading: " + filename)
        url = "https://github.com/virtualxt/ProcessorTests/raw/main/8088/v1/" + filename
        fp = open(filepath, "wb")
        fp.write(requests.get(url).content)
        fp.close()

def gen_tests(opcode, sub = -1):
    for arg in os.sys.argv[1:]:
        if int(arg, 16) != opcode:
            return
        
    if sub < 0: sub = ""
    else: sub = "." + str(sub)

    filename = "{:02X}{}.json.gz".format(opcode, sub)
    check_and_download(filename)
    print("Generating tests for opcode {:02X}...".format(opcode))
    
    output_file = "tools/tests/8088v1/{:02X}{}.bin".format(opcode, sub)

    with open(c_test_name, "a") as f:
        f.write(c_body.format(name = "opcode_{:02X}{}".format(opcode, sub.replace(".", "_")), input = output_file))

    output = open(output_file, "wb")
    output.write(struct.pack("I", 0))
    
    num_tests = 0
    with gzip.open("tools/tests/8088v1/" + filename, mode="rt") as f:
        data = json.loads(f.read())
        for test in data:
            opcode_ref = mask_file["{:02X}".format(opcode)]
            if sub != "":
                opcode_ref = opcode_ref["reg"][sub[1:]]

            flags_mask = 0xFFFF
            if "flags-mask" in opcode_ref:
                flags_mask = int(opcode_ref["flags-mask"])
            output.write(struct.pack("H", flags_mask))

            write_test(test, output)
            num_tests += 1

    output.seek(0, 0)
    output.write(struct.pack("I", num_tests))
    output.close()

####################### Start #######################

with open(c_test_name, "w") as f: f.write(c_header)

opcodes_bugs = {
    0x11, 0x17, 0x27, 0x2F, 0x37, 0x3F, 0x50, 0x54, 0x57, 0x5A, 0x5D,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x86, 0x87, 0x8D, 0xA5, 0xAB, 0xC0, 0xC1, 0xC4, 0xC8, 0xC9, 0xCF,
    0xD4, 0xD5, 0xE5
}

for i in range(256):
    if i in opcodes_bugs:
        continue
    skip_opcodes = {
        0x26, 0x2E, 0x36, 0x3E, 0x70, 0x80, 0x81,
        0x82, 0x83, 0x9B, 0xD0, 0xD1, 0xD2, 0xD3,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF6, 0xF7,
        0xFE, 0xFF
    }
    if i in skip_opcodes:
        continue
    gen_tests(i)

for i in range(8):
    gen_tests(0x80, i)
    gen_tests(0x81, i)
    gen_tests(0x82, i)
    gen_tests(0x83, i)
    #gen_tests(0xD0, i)
    #gen_tests(0xD1, i)
    #gen_tests(0xD2, i)
    #gen_tests(0xD3, i)
    #gen_tests(0xF6, i)
    #gen_tests(0xF7, i)
    #gen_tests(0xFF, i)

gen_tests(0xFE, 0)
gen_tests(0xFE, 1)
