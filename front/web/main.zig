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
const std = @import("std");

export fn wasm_video_rgba_memory_pointer() [*]const u8 {
    return @ptrCast([*]const u8, c.video_rgba_memory_pointer());
}

export fn wasm_video_width() i32 {
    return @as(i32, c.video_width());
}

export fn wasm_video_height() i32 {
    return @as(i32, c.video_height());
}

export fn wasm_step_emulation(cycles: i32) void {
    c.step_emulation(@as(c_int, cycles));
}

export fn wasm_initialize_emulator() void {
    c.initialize_emulator();
}

// -------- Embedded files --------

const pcxtbios = @embedFile("../../bios/pcxtbios.bin");
const vxtx = @embedFile("../../bios/vxtx.bin");
const freedos_hd = @embedFile("../../boot/freedos_hd.img");

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

export fn get_freedos_hd_data() [*]const u8 {
    return freedos_hd;
}

export fn get_freedos_hd_size() u32 {
    return freedos_hd.len;
}
