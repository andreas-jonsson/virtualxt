#!/usr/bin/env python

import gzip
import json
import os
import requests
import struct

c_header = """// This file is generated!

#include <vxt/vxtu.h>
#include "testing.h"

VXT_PACK(struct) registers {{
    vxt_word ax, bx, cx, dx;
    vxt_word cs, ss, ds, es;
    vxt_word sp, bp, si, di;
    vxt_word ip, flags;
}};

VXT_PACK(struct) memory {{
    vxt_dword addr;
    vxt_byte data;
}};

#define NAME_SIZE 256
#define CHECK_REG(reg) TASSERT(r->reg == regs.reg, "Expected the value of register '" #reg "' to be 0x%X(%d) but it was 0x%X(%d)", regs.reg, regs.reg, r->reg, r->reg)

#ifdef TESTING

static int execute_test(struct Test T, int *index, char *name, const char *input) {{
    struct vxt_peripheral *devices[] = {{
        vxtu_memory_create(&TALLOC, 0x0, 0x100000, false),
        NULL
    }};

    vxt_system *s = vxt_system_create(&TALLOC, VXT_DEFAULT_FREQUENCY, devices);
    TENSURE(s);
    TENSURE_NO_ERR(vxt_system_initialize(s));

    FILE *fp = fopen(input, "rb");
    TENSURE(fp);

    int num_tests;
    TENSURE(fread(&num_tests, 4, 1, fp) == 1);

    for (int i = 0; i < num_tests; i++) {{
        *index = i;

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

		// INT0
		vxt_system_write_byte(s, 0, 0);
		vxt_system_write_byte(s, 1, 0x40);
		vxt_system_write_byte(s, 2, 0);
		vxt_system_write_byte(s, 3, 0);

        vxt_word num_mem;
        TENSURE(fread(&num_mem, 2, 1, fp) == 1);

        for (int i = 0; i < (int)num_mem; i++) {{
            struct memory mem;
            TENSURE(fread(&mem, sizeof(struct memory), 1, fp) == 1);
            vxt_system_write_byte(s, mem.addr, mem.data);
        }}

		bool unsupported_prefix = strstr(name, "repc") || strstr(name, "repnc");
		struct vxt_step step = {{0}};
		
		if (!unsupported_prefix) {{
			step = vxt_system_step(s, 0);
			TENSURE_NO_ERR(step.err);
		}}

		TENSURE(fread(&regs, sizeof(struct registers), 1, fp) == 1);

		if (!unsupported_prefix) {{
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
		}}
        
        // Not sure if we should have flags here defined.
        if (!step.interrupt && !unsupported_prefix)
            TASSERT((r->flags & flags_mask) == (regs.flags & flags_mask), "Expected flags register to be 0x%X but it was 0x%X", (regs.flags & flags_mask), (r->flags & flags_mask));

        TENSURE(fread(&num_mem, 2, 1, fp) == 1);

        for (int i = 0; i < (int)num_mem; i++) {{
            struct memory mem;
            TENSURE(fread(&mem, sizeof(struct memory), 1, fp) == 1);
            
            // If there was an interrupt, undefined flags might be on the stack.
            if (!step.interrupt && !unsupported_prefix) {{
                vxt_byte data = vxt_system_read_byte(s, mem.addr);
                TASSERT(data == mem.data, "Expected memory at address 0x%X to be 0x%X(%d) but it is 0x%X(%d)", mem.addr, mem.data, mem.data, data, data);
            }}
        }}

        vxt_word cycles;
        TENSURE(fread(&cycles, 2, 1, fp) == 1);
        if ({cycles} && cycles != step.cycles)
            TLOG("%d: Expected \\"%s\\" to execute in %d cycles, but it took %d!", i, name, cycles, step.cycles);

        // Just skip the queue size for now.
        TENSURE(fread(&num_mem, 1, 1, fp) == 1);
    }}

    vxt_system_destroy(s);
    fclose(fp);
    return 0;
}}

#endif
"""

c_body = """
TEST({name},
    int index = 0;
    char name[NAME_SIZE] = {{0}};
    if (execute_test(T, &index, name, "{input}") != 0) {{
        TLOG("%d: %s", index, name);
        return -1;
    }}
)
"""

def pack_test(data, alt, output):
    regs = data["regs"]
    if alt:
        for name,reg in alt["regs"].items():
            if name not in regs:
                regs[name] = reg
    
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
    pack_test(data["initial"], None, output)
    pack_test(data["final"], data["initial"], output)
    output.write(struct.pack("HB", len(data["cycles"]), len(data["final"]["queue"])))

def check_and_download(filename):
    filepath = data_dir + "/" + filename
    if not os.path.exists(filepath):
        print("Downloading: " + filename)

        url = "https://github.com/virtualxt/v20/raw/main/v1_native/" + filename
        resp = requests.get(url)
        if resp.status_code != requests.codes.ok:
            return False

        with open(filepath, "wb") as f:
            f.write(resp.content)

    return True

def gen_tests(input_name):
    opcode = int(input_name[:2], 16)
    reg = -1

    if len(input_name) > 2:
        reg = int(input_name[3:])

    for op in skip_opcodes:
        if isinstance(op, tuple):
            if op[0] == opcode and reg == op[1]:
                return
        elif op == opcode:
            return

    for arg in os.sys.argv[1:]:
        if int(arg, 16) != opcode:
            return

    filename = input_name + ".json.gz"
    if not check_and_download(filename):
        print("Could not download: " + filename)
        return

    print("Generating tests for opcode {}".format(input_name))

    output_file = "{}/{}.bin".format(data_dir, input_name)

    with open(c_test_name, "a") as f:
        f.write(c_body.format(name = "opcode_V20_" + input_name.replace(".", "_"), input = output_file))

    output = open(output_file, "wb")
    output.write(struct.pack("I", 0))

    num_tests = 0
    with gzip.open(data_dir + "/" + filename, mode="r") as f:
        data = json.loads(f.read())
        for test in data:
            opcode_ref = index_file["{:02X}".format(opcode)]
            if reg >= 0:
                opcode_ref = opcode_ref["reg"][str(reg)]

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

c_test_name = "lib/vxt/v20_tests.c"
data_dir = "tools/tests/V20v1"

intel_186_opcodes = (
	0x60, 0x61, 0x62, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0xC0, 0xC1, 0xC8, 0xC9,
)

skip_opcodes = (
)

check_and_download("metadata.json")
index_file = json.loads(open(data_dir + "/metadata.json", "r").read())["opcodes"]

check_cycles = "false"
with open(c_test_name, "w") as f: f.write(c_header.format(cycles = check_cycles))

for opcode,data in index_file.items():
    if not int(opcode, 16) in intel_186_opcodes:
        continue
        
    if "reg" in data:
        for reg in data["reg"]:
            gen_tests("{}.{}".format(opcode, reg))
    else:
        gen_tests(opcode)
