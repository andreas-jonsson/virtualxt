// Copyright (c) 2019-2022 Andreas T Jonsson
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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxtu.h>

#include "common.h"
#include "system.h"
#include "testing.h"

#define RUN_BBTEST(bin, check, ...) {                                                                   \
    struct vxt_pirepheral *ram = vxtu_memory_create(TALLOC, 0x0, 0x100000, false);                      \
    struct vxt_pirepheral *rom = vxtu_memory_create(TALLOC, 0xF0000, 0x10000, true);                    \
                                                                                                        \
    int size = 0;                                                                                       \
    vxt_byte *data = vxtu_read_file(TALLOC, (bin), &size);                                              \
    TENSURE(data);                                                                                      \
    TENSURE(vxtu_memory_device_fill(rom, data, size));                                                  \
    TFREE(data);                                                                                        \
                                                                                                        \
    struct vxt_pirepheral *devices[] = {                                                                \
        ram, rom,                                                                                       \
        NULL                                                                                            \
    };                                                                                                  \
                                                                                                        \
    CONSTP(vxt_system) s = vxt_system_create(TALLOC, devices);                                          \
    TENSURE_NO_ERR(vxt_system_initialize(s));                                                           \
    vxt_system_reset(s);                                                                                \
    struct vxt_registers *r = vxt_system_registers(s);                                                  \
    r->cs = 0xF000;                                                                                     \
    r->ip = 0xFFF0;                                                                                     \
                                                                                                        \
    for (;;) {                                                                                          \
        struct vxt_step step = vxt_system_step(s, 0);                                                   \
        TENSURE_NO_ERR(step.err);                                                                       \
        if (step.halted)                                                                                \
            break;                                                                                      \
    }                                                                                                   \
                                                                                                        \
    check(__VA_ARGS__);                                                                                 \
}                                                                                                       \

//#define DIFF_LOG TLOG("Diff @ 0x%X: Expected 0x%X but found 0x%X.", i, res_data[i], v)
#define DIFF_LOG

#define CHECK_DIFF(res, diff) {                                                                         \
    vxt_byte *res_data = vxtu_read_file(TALLOC, (res), &size);                                          \
    TENSURE(res_data);                                                                                  \
                                                                                                        \
    int diff_count = 0;                                                                                 \
    for (int i = 0; i < size; i++) {                                                                    \
        vxt_byte v = vxt_system_read_byte(s, (vxt_pointer)i);                                           \
        if (v != res_data[i]) {                                                                         \
            DIFF_LOG;                                                                                   \
            diff_count++;                                                                               \
        }                                                                                               \
    }                                                                                                   \
    TFREE(res_data);                                                                                    \
                                                                                                        \
    vxt_system_destroy(s);                                                                              \
    TASSERT(diff_count == (diff), "Diff is %d, expected %d.", diff_count, (diff));                      \
}                                                                                                       \

#define COMP_MEM_CHECK(addr, type, value) { TENSURE(vxt_system_read_ ## type (s, (vxt_pointer)(addr)) == (value)); vxt_system_destroy(s); }
#define NO_CHECK(_) vxt_system_destroy(s)

TEST(blackbox_add,
    RUN_BBTEST("tools/testdata/add.bin", CHECK_DIFF, "tools/testdata/res_add.bin", 0);
)

TEST(blackbox_bcdcnv,
    RUN_BBTEST("tools/testdata/bcdcnv.bin", CHECK_DIFF, "tools/testdata/res_bcdcnv.bin", 9);
)

TEST(blackbox_bitwise,
    RUN_BBTEST("tools/testdata/bitwise.bin", CHECK_DIFF, "tools/testdata/res_bitwise.bin", 0);
)

TEST(blackbox_cmpneg,
    RUN_BBTEST("tools/testdata/cmpneg.bin", CHECK_DIFF, "tools/testdata/res_cmpneg.bin", 0);
)

TEST(blackbox_control,
    RUN_BBTEST("tools/testdata/control.bin", CHECK_DIFF, "tools/testdata/res_control.bin", 0);
)

TEST(blackbox_datatrnf,
    RUN_BBTEST("tools/testdata/datatrnf.bin", CHECK_DIFF, "tools/testdata/res_datatrnf.bin", 0);
)

TEST(blackbox_div,
    RUN_BBTEST("tools/testdata/div.bin", CHECK_DIFF, "tools/testdata/res_div.bin", 7);
)

TEST(blackbox_interrupt,
    RUN_BBTEST("tools/testdata/interrupt.bin", CHECK_DIFF, "tools/testdata/res_interrupt.bin", 0);
)

TEST(blackbox_jmpmov,
    RUN_BBTEST("tools/testdata/jmpmov.bin", COMP_MEM_CHECK, 0, word, 0x4001);
)

TEST(blackbox_jump1,
    RUN_BBTEST("tools/testdata/jump1.bin", CHECK_DIFF, "tools/testdata/res_jump1.bin", 0);
)

TEST(blackbox_jump2,
    RUN_BBTEST("tools/testdata/jump2.bin", CHECK_DIFF, "tools/testdata/res_jump2.bin", 0);
)

TEST(blackbox_mul,
    RUN_BBTEST("tools/testdata/mul.bin", CHECK_DIFF, "tools/testdata/res_mul.bin", 8);
)

TEST(blackbox_rep,
    RUN_BBTEST("tools/testdata/rep.bin", CHECK_DIFF, "tools/testdata/res_rep.bin", 0);
)

TEST(blackbox_rotate,
    RUN_BBTEST("tools/testdata/rotate.bin", CHECK_DIFF, "tools/testdata/res_rotate.bin", 0);
)

TEST(blackbox_segpr,
    RUN_BBTEST("tools/testdata/segpr.bin", CHECK_DIFF, "tools/testdata/res_segpr.bin", 0);
)

TEST(blackbox_shift,
    RUN_BBTEST("tools/testdata/shifts.bin", CHECK_DIFF, "tools/testdata/res_shifts.bin", 0);
)

TEST(blackbox_strings,
    RUN_BBTEST("tools/testdata/strings.bin", CHECK_DIFF, "tools/testdata/res_strings.bin", 0);
)

TEST(blackbox_sub,
    RUN_BBTEST("tools/testdata/sub.bin", CHECK_DIFF, "tools/testdata/res_sub.bin", 0);
)
