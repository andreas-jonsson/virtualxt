#!/bin/env python

import gzip
import json
import os
import requests
import shutil

file_header = """// This file is generated!

#include <vxt/vxtu.h>
#include "../testing.h"
"""

file_begin = """
static int {test_name}(struct Test T) {{
    struct vxt_pirepheral *devices[] = {{
        vxtu_memory_create(&realloc, 0x0, 0x100000, false),
        NULL
    }};

    vxt_system *s = vxt_system_create(&realloc, VXT_CPU_8088, VXT_DEFAULT_FREQUENCY, devices);
    TENSURE(s);
    TENSURE_NO_ERR(vxt_system_initialize(s));

    TLOG("Testing: {name}");

"""

file_body = """
    vxt_system_reset(s);
    struct vxt_registers *r = vxt_system_registers(s);

    r->ax = {ax};
    r->bx = {bx};
    r->cx = {cx};
    r->dx = {dx};

    r->cs = {cs};
    r->ss = {ss};
    r->ds = {ds};
    r->es = {es};

    r->sp = {sp};
    r->bp = {bp};
    r->si = {si};
    r->di = {di};

    r->ip = {ip};
    r->flags = {flags};
"""

file_step = """
    struct vxt_step step = vxt_system_step(s, 0);
    TENSURE_NO_ERR(step.err);

    #define CHECK_MEM(addr, value) TASSERT(vxt_system_read_byte(s, (addr)) == (value), "ERROR: Memory at 0x%X is not equal to %d!\\n", (addr), (value));
"""

file_check_reg = """
    #define CHECK_REG(reg, res) TASSERT(r->reg == (res), "ERROR: " #reg " is not equal to %d!\\n", (res));

    CHECK_REG(ax, {ax});
    CHECK_REG(bx, {bx});
    CHECK_REG(cx, {cx});
    CHECK_REG(dx, {dx});

    CHECK_REG(cs, {cs});
    CHECK_REG(ss, {ss});
    CHECK_REG(ds, {ds});
    CHECK_REG(es, {es});

    CHECK_REG(sp, {sp});
    CHECK_REG(bp, {bp});
    CHECK_REG(si, {si});
    CHECK_REG(di, {di});

    CHECK_REG(ip, {ip});
    TASSERT((r->flags & flags_mask) == ({flags} & flags_mask), "ERROR: Flags is not equal to 0x%X!\\n", ({flags} & flags_mask));

"""

file_end = """
    vxt_system_destroy(s);
    return 0;
}}

TEST({test_name},
    {test_name}(T);
)
"""

mask_file = json.loads(open("tools/tests/8088v1/8088.json", "r").read())

def write_test(data, test_name, flags_mask, output):
    initial = data["initial"]
    final = data["final"]

    output.write(file_begin.format(test_name = test_name, name = data["name"]))

    output.write("\tconst vxt_word flags_mask = " + flags_mask + ";\n")

    for mem in initial["ram"]:
        output.write("\tvxt_system_write_byte(s, {0}, {1});\n".format(mem[0], mem[1]))

    regs = initial["regs"]
    output.write(file_body.format(
        ax = regs["ax"],
        bx = regs["bx"],
        cx = regs["cx"],
        dx = regs["dx"],

        cs = regs["cs"],
        ss = regs["ss"],
        ds = regs["ds"],
        es = regs["es"],

        sp = regs["sp"],
        bp = regs["bp"],
        si = regs["si"],
        di = regs["di"],

        ip = regs["ip"],
        flags = regs["flags"]
    ))

    output.write(file_step)

    regs = final["regs"]
    output.write(file_check_reg.format(
        ax = regs["ax"],
        bx = regs["bx"],
        cx = regs["cx"],
        dx = regs["dx"],

        cs = regs["cs"],
        ss = regs["ss"],
        ds = regs["ds"],
        es = regs["es"],

        sp = regs["sp"],
        bp = regs["bp"],
        si = regs["si"],
        di = regs["di"],

        ip = regs["ip"],
        flags = regs["flags"]
    ))

    for mem in final["ram"]:
        output.write("\tCHECK_MEM({0}, {1});\n".format(mem[0], mem[1]))

    output.write(file_end.format(test_name = test_name))

def check_and_download(filename):
    filepath = "tools/tests/8088v1/" + filename
    if not os.path.exists(filepath):
        print("Downloading: " + filename)
        url = "https://github.com/virtualxt/ProcessorTests/raw/main/8088/v1/" + filename
        fp = open(filepath, "wb")
        fp.write(requests.get(url).content)
        fp.close()

def gen_tests(opcode, sub):
    for arg in os.sys.argv[1:]:
        if int(arg, 16) != opcode:
            return

    filename = "{:02X}{}.json.gz".format(opcode, sub)
    check_and_download(filename)
    print("Generating tests for opcode {:02X}...".format(opcode))

    sub_sym = sub.replace(".", "_")
    output = open("lib/vxt/test/opcode_{:02X}{}.c".format(opcode, sub_sym), "w")
    output.write(file_header)

    with gzip.open("tools/tests/8088v1/" + filename, mode="rt") as f:
        data = json.loads(f.read())
        for num,test in enumerate(data):
            opcode_ref = mask_file["{:02X}".format(opcode)]
            if sub != "":
                opcode_ref = opcode_ref["reg"][sub[1:]]

            flags_mask = "0xFFFF"
            if "flags-mask" in opcode_ref:
                flags_mask = "0x{:04X};\n".format(opcode_ref["flags-mask"])

            write_test(test, "opcode_{:02X}{}_{}".format(opcode, sub_sym, num), flags_mask, output)

    output.close()

####################### Start #######################

test_output_dir = "lib/vxt/test"
if os.path.exists(test_output_dir):
    shutil.rmtree(test_output_dir)
os.mkdir(test_output_dir)

ignore_opcodes = {
    0x26, 0x2E, 0x36, 0x3E, 0x70, 0x80, 0x81,
    0x82, 0x83, 0x9B, 0xD0, 0xD1, 0xD2, 0xD3,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF6, 0xF7,
    0xFE, 0xFF
}

for i in range(256):
    if i in ignore_opcodes:
        continue
    gen_tests(i, "")

for i in range(8):
    grp = "." + str(i)
    gen_tests(0x80, grp)
    gen_tests(0x81, grp)
    gen_tests(0x82, grp)
    gen_tests(0x83, grp)
    gen_tests(0x83, grp)
    gen_tests(0xD0, grp)
    gen_tests(0xD1, grp)
    gen_tests(0xD2, grp)
    gen_tests(0xD3, grp)
    gen_tests(0xF6, grp)
    gen_tests(0xF7, grp)
    gen_tests(0xFF, grp)

gen_tests(0xFE, ".0")
gen_tests(0xFE, ".1")