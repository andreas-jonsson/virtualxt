// Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

const c = @cImport(@cInclude("main.h"));

extern var __bss_start: u8;
extern var __bss_end: u8;

comptime {
    asm (
        \\.section .text.boot
        \\.globl _start
        \\_start:
    );

    asm (
        \\ mrs x0,mpidr_el1
        \\ mov x1,#0xC1000000
        \\ bic x0,x0,x1
        \\ cbz x0,master
        \\hang:
        \\ wfe
        \\ b hang
        \\master:
        \\ mov sp,#0x08000000
        \\ mov x0,#0x1000 //exception_vector_table
        \\ msr vbar_el3,x0
        \\ msr vbar_el2,x0
        \\ msr vbar_el1,x0
        \\ bl main
        \\.balign 0x800
        \\.section .text.exception_vector_table
        \\exception_vector_table:
        \\.balign 0x80
        \\ b exceptionEntry0x00
        \\.balign 0x80
        \\ b exceptionEntry0x01
        \\.balign 0x80
        \\ b exceptionEntry0x02
        \\.balign 0x80
        \\ b exceptionEntry0x03
        \\.balign 0x80
        \\ b exceptionEntry0x04
        \\.balign 0x80
        \\ b exceptionEntry0x05
        \\.balign 0x80
        \\ b exceptionEntry0x06
        \\.balign 0x80
        \\ b exceptionEntry0x07
        \\.balign 0x80
        \\ b exceptionEntry0x08
        \\.balign 0x80
        \\ b exceptionEntry0x09
        \\.balign 0x80
        \\ b exceptionEntry0x0A
        \\.balign 0x80
        \\ b exceptionEntry0x0B
        \\.balign 0x80
        \\ b exceptionEntry0x0C
        \\.balign 0x80
        \\ b exceptionEntry0x0D
        \\.balign 0x80
        \\ b exceptionEntry0x0E
        \\.balign 0x80
        \\ b exceptionEntry0x0F
    );
}

export fn nop() void {
    asm volatile ("nop");
}


pub fn setcntfrq(word: u32) void {
    asm volatile ("msr cntfrq_el0, %[word]"
        :
        : [word] "{x0}" (word)
    );
}

export fn cntfrq() u32 {
    var word: usize = asm volatile ("mrs %[word], cntfrq_el0"
        : [word] "=r" (-> usize)
    );
    return @truncate(u32, word);
}

export fn cntpct32() u32 {
    var word: usize = asm volatile ("mrs %[word], cntpct_el0"
        : [word] "=r" (-> usize)
    );
    return @truncate(u32, word);
}

export fn main() noreturn {
    while (true) {
        //setcntfrq(1000000); // This breaks QEMU?
        @memset(@as(*volatile [1]u8, &__bss_start), 0, @ptrToInt(&__bss_end) - @ptrToInt(&__bss_start));
        _ = c.c_main(0, null);
    }
}

export fn exceptionEntry0x00() noreturn {
    exceptionHandler(0x00);
}

export fn exceptionEntry0x01() noreturn {
    exceptionHandler(0x01);
}

export fn exceptionEntry0x02() noreturn {
    exceptionHandler(0x02);
}

export fn exceptionEntry0x03() noreturn {
    exceptionHandler(0x03);
}

export fn exceptionEntry0x04() noreturn {
    exceptionHandler(0x04);
}

export fn exceptionEntry0x05() noreturn {
    exceptionHandler(0x05);
}

export fn exceptionEntry0x06() noreturn {
    exceptionHandler(0x06);
}

export fn exceptionEntry0x07() noreturn {
    exceptionHandler(0x07);
}

export fn exceptionEntry0x08() noreturn {
    exceptionHandler(0x08);
}

export fn exceptionEntry0x09() noreturn {
    exceptionHandler(0x09);
}

export fn exceptionEntry0x0A() noreturn {
    exceptionHandler(0x0A);
}

export fn exceptionEntry0x0B() noreturn {
    exceptionHandler(0x0B);
}

export fn exceptionEntry0x0C() noreturn {
    exceptionHandler(0x0C);
}

export fn exceptionEntry0x0D() noreturn {
    exceptionHandler(0x0D);
}

export fn exceptionEntry0x0E() noreturn {
    exceptionHandler(0x0E);
}

export fn exceptionEntry0x0F() noreturn {
    exceptionHandler(0x0F);
}

fn exceptionHandler(entry_number: u32) noreturn {
    c.exception_handler(entry_number);
    while (true) {}
}

// -------- Embedded files --------

const disk = @embedFile("../../boot/freedos.img");
const pcxtbios = @embedFile("../../bios/pcxtbios.bin");
const vxtx = @embedFile("../../bios/vxtx.bin");

export fn get_disk_data() [*]const u8 {
    return disk;
}

export fn get_disk_size() u32 {
    return disk.len;
}

export fn get_pcxtbios_data() [*]const u8 {
    return pcxtbios;
}

export fn get_pcxtbios_size() u32 {
    return pcxtbios.len;
}

export fn get_vxtx_data() [*]const u8 {
    return vxtx;
}

export fn get_vxtx_size() u32 {
    return vxtx.len;
}
