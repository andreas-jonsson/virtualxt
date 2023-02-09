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

const std = @import("std");
const builtin = @import("builtin");

const Builder = std.build.Builder;
const print = std.debug.print;

const c_options = &[_][]const u8{
    "-Wall",
    "-Wextra",
    "-Werror",

    // Looks like there is some kind of bug that prevents you from only using 'unsigned-integer-overflow'.
    // Normally 'signed-integer-overflow' should be sanitized.
    "-fno-sanitize=signed-integer-overflow",
    "-fno-sanitize=unsigned-integer-overflow",
};

const CTest = struct {
    name: []const u8,
    offset: usize
};

fn find_test(buffer: []const u8, start: usize) ?CTest {
    const prefix = std.mem.indexOfPos(u8, buffer, start, "\nTEST(") orelse return null;
    const sufix = std.mem.indexOfPos(u8, buffer, prefix, ",") orelse return null;
    return CTest{.name = buffer[prefix+6..sufix], .offset = sufix};
}

fn create_test_files() anyerror!void {
    const cwd = std.fs.cwd();
    try cwd.makePath("test");

    var h_out = try cwd.createFile("test/test.h", .{.truncate = true});
    defer h_out.close();
    try h_out.writer().print("#include <testing.h>\n\n", .{});

    var zig_out = try cwd.createFile("test/test.zig", .{.truncate = true});
    defer zig_out.close();
    try zig_out.writer().print("const std = @import(\"std\");\nconst c = @cImport({{\n    @cInclude(\"testing.h\");\n    @cInclude(\"test.h\");\n}});\n\n", .{});
}

fn parse_file(name: []const u8) anyerror!void {
    var gp = std.heap.GeneralPurposeAllocator(.{.safety = true}){};
    defer _ = gp.deinit();
    const allocator = gp.allocator();

    const cwd = std.fs.cwd();

    var file = try cwd.openFile(name, .{});
    defer file.close();

    const megabyte = 1024*1024;
    const file_buffer = try file.readToEndAlloc(allocator, megabyte);
    defer allocator.free(file_buffer);

    var h_out = try cwd.openFile("test/test.h", .{.mode = .read_write});
    defer h_out.close();
    try h_out.seekFromEnd(0);

    var zig_out = try cwd.openFile("test/test.zig", .{.mode = .read_write});
    defer zig_out.close();
    try zig_out.seekFromEnd(0);

    var start: usize = 0;
    while (true) {
        const test_name = find_test(file_buffer, start) orelse break;
        try h_out.writer().print("extern int test_{s}(struct Test T);\n", .{test_name.name});
        try zig_out.writer().print("test \"{s}\" {{\n    try std.testing.expect(c.run_test(c.test_{s}));\n}}\n", .{test_name.name, test_name.name});
        start = test_name.offset;
    }
}

fn build_libvxt(b: *Builder, mode: std.builtin.Mode, target: std.zig.CrossTarget, cpu286: bool, cpuV20: bool, testing: bool) *std.build.LibExeObjStep {
    const libname = b.fmt("vxt.{s}.{s}", .{@tagName(target.getOsTag()), @tagName(target.getCpuArch())});
    const lib = b.addStaticLibrary(libname, null);

    lib.addIncludePath("lib/vxt/include");
    lib.setTarget(target);

    if (cpu286 or testing) {
        lib.defineCMacroRaw("VXT_CPU_286");
    } else if (cpuV20) {
        lib.defineCMacroRaw("VXT_CPU_V20");
    }

    if (testing) {
        lib.defineCMacroRaw("TESTING");
        lib.linkLibC();
        lib.setBuildMode(.ReleaseSafe);
        create_test_files() catch unreachable;
    } else {
        lib.setBuildMode(mode);
        if (!target.toTarget().isWasm()) {
            lib.setOutputDir("build/lib");
            lib.install();
        }
    }

    const opt = c_options ++ &[_][]const u8{
        "-Wstrict-prototypes",
        "-Wcast-align",
        "-Wno-unused-function", // Needed for unit tests.
        "-std=c11",
        "-pedantic",
        //"-fPIC", // Needed for libretro.
    };

    const files = &[_][]const u8{
        "lib/vxt/system.c",
        "lib/vxt/dummy.c",
        "lib/vxt/memory.c",
        "lib/vxt/cpu.c",
        "lib/vxt/debugger.c",
    };

    for (files) |file| {
        lib.addCSourceFile(file, opt);
        if (testing) {
            parse_file(file) catch unreachable;
        }
    }
    if (testing) {
        const file = "lib/vxt/testsuit.c";
        lib.addCSourceFile(file, opt);
        parse_file(file) catch unreachable;
    }
    return lib;
}

pub fn build(b: *Builder) void {
    const validator = b.option(bool, "validator", "Enable PI8088 hardware validator") orelse false;
    const network = b.option(bool, "pcap", "Link with libpcap") orelse false;
    const sdl_path = b.option([]const u8, "sdl-path", "Path to SDL2 headers and libs") orelse null;
    const cpu286 = b.option(bool, "at", "Enable Intel 286 and IBM AT support") orelse false;
    const cpuV20 = b.option(bool, "v20", "Enable NEC V20 CPU support") orelse false;

    const mode = b.standardReleaseOptions();
    const target = b.standardTargetOptions(.{});

    // -------- libvxt --------

    const libvxt = build_libvxt(b, mode, target, cpu286, cpuV20, false);
    b.step("lib", "Build libvxt").dependOn(&libvxt.step);

    // -------- ini --------

    const ini = b.addStaticLibrary("ini", "lib/inih/ini.c");
    ini.setBuildMode(mode);
    ini.setTarget(target);
    ini.linkLibC();

    // -------- nuked-opl3 --------

    const opl3 = b.addStaticLibrary("nuked-opl3", "lib/nuked-opl3/opl3.c");
    opl3.setBuildMode(mode);
    opl3.setTarget(target);
    opl3.linkLibC();

    // -------- microui --------

    const microui = b.addStaticLibrary("microui", null);
    microui.setBuildMode(mode);
    microui.setTarget(target);
    microui.linkLibC();
    microui.addIncludePath("lib/microui/src");
    microui.addCSourceFile("lib/microui/src/microui.c", c_options ++ &[_][]const u8{"-std=c11", "-pedantic"});

    // -------- pirepheral --------

    const pirepheral = b.addStaticLibrary("vxtp", null);
    {
        pirepheral.setBuildMode(mode);
        pirepheral.setTarget(target);
        pirepheral.linkLibC();
        pirepheral.addIncludePath("lib/vxtp");
        pirepheral.addIncludePath("lib/vxt/include");

        if (validator) {
            pirepheral.defineCMacroRaw("PI8088");
        }
        
        if (cpu286) {
            pirepheral.defineCMacroRaw("VXT_CPU_286");
            pirepheral.defineCMacroRaw("VXTP_SYS_AT");
        } else if (cpuV20) {
            pirepheral.defineCMacroRaw("VXT_CPU_V20");
        }

        const opt = c_options ++ &[_][]const u8{"-std=c11", "-pedantic"};
        pirepheral.addCSourceFile("lib/vxtp/disk.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/pic.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/ppi.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/pit.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/dma.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/mda.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/cga.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/vga.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/rifs.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/mouse.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/serial_dbg.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/joystick.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/rtc.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/adlib.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/fdc.c", opt);
        pirepheral.addCSourceFile("lib/vxtp/ctrl.c", opt);

        if (network) {
            pirepheral.defineCMacroRaw("VXTP_NETWORK");
            pirepheral.addCSourceFile("lib/vxtp/network.c", opt);
            pirepheral.linkSystemLibrary("pcap");
        }

        pirepheral.linkLibrary(opl3);
        pirepheral.defineCMacroRaw("VXTP_NUKED_OPL3");
        pirepheral.addIncludePath("lib/nuked-opl3");
    }

    // -------- virtualxt sdl --------

    const exe_sdl = b.addExecutable("virtualxt", "front/sdl/main.zig");
    exe_sdl.addIncludePath("front/sdl");
    exe_sdl.defineCMacroRaw("ENTRY=c_main"); 

    exe_sdl.setBuildMode(mode);
    exe_sdl.setTarget(target);
    exe_sdl.setOutputDir("build/bin");
    exe_sdl.defineCMacroRaw(b.fmt("PLATFORM={s}", .{@tagName(target.getOsTag())}));

    if (sdl_path != null) {
        const p = .{sdl_path orelse ""}; // Make compiler happy.
        print("SDL2 location: {s}\n", p);

        exe_sdl.addIncludePath(b.fmt("{s}/include", p));

        if (target.isWindows() and target.getAbi() == .gnu) {
            exe_sdl.addLibraryPath(b.fmt("{s}/lib/x64", p));
        } else {
            exe_sdl.addLibraryPath(b.fmt("{s}/lib", p));
        }
    }

    exe_sdl.linkSystemLibrary("SDL2");

    // This is a hack for FreeBSD. SDL2 is missing symbols from these libs.
    if (target.getOsTag() == .freebsd) {
        exe_sdl.linkSystemLibrary("usbhid");
        exe_sdl.linkSystemLibrary("inotify");
    }

    exe_sdl.linkLibrary(libvxt);
    exe_sdl.addIncludePath("lib/vxt/include");

    exe_sdl.linkLibrary(pirepheral);
    exe_sdl.addIncludePath("lib/vxtp");

    exe_sdl.linkLibrary(ini);
    exe_sdl.addIncludePath("lib/inih");

    exe_sdl.linkLibrary(microui);
    exe_sdl.addIncludePath("lib/microui/src");

    exe_sdl.linkLibrary(opl3); // Part of vxtp?

    exe_sdl.linkLibC();

    const opt = c_options ++ &[_][]const u8{"-std=c11", "-pedantic"};
    exe_sdl.addCSourceFile("front/sdl/main.c", opt);
    exe_sdl.addCSourceFile("front/sdl/mu_renderer.c", opt);
    exe_sdl.addCSourceFile("front/sdl/window.c", opt);
    exe_sdl.addCSourceFile("front/sdl/docopt.c", &[_][]const u8{"-std=c11", "-Wno-unused-variable", "-Wno-unused-parameter"});

    if (cpu286) {
        exe_sdl.defineCMacroRaw("VXT_CPU_286");
        exe_sdl.defineCMacroRaw("VXTP_SYS_AT");
    } else if (cpuV20) {
        exe_sdl.defineCMacroRaw("VXT_CPU_V20");
    }

    if (validator) {
        exe_sdl.linkSystemLibrary("gpiod");
        exe_sdl.defineCMacroRaw("PI8088");
        exe_sdl.addCSourceFile("tools/validator/pi8088/pi8088.c", opt);
    }

    if (network) {
        exe_sdl.defineCMacroRaw("VXTP_NETWORK");
    }

    // -------- virtualxt libretro --------

    {
        const libname = b.fmt("vxt-libretro.{s}.{s}", .{@tagName(target.getOsTag()), @tagName(target.getCpuArch())});
        const libretro = b.addSharedLibrary(libname, "front/libretro/files.zig", .unversioned);
        libretro.setBuildMode(mode);
        libretro.setTarget(target);
        libretro.setOutputDir("build/lib");
        libretro.setMainPkgPath("."); // Needed for embedded files.

        libretro.linkLibrary(build_libvxt(b, mode, target, cpu286, cpuV20, false));
        libretro.addIncludePath("lib/vxt/include");

        libretro.linkLibrary(pirepheral);
        libretro.addIncludePath("lib/vxtp");

        libretro.linkLibC();

        libretro.addIncludePath("lib/libretro");

        libretro.addCSourceFile("front/libretro/core.c", c_options ++ &[_][]const u8{"-std=c11", "-pedantic"});

        b.step("libretro", "Build libretro core").dependOn(&libretro.step);
    }

    // -------- virtualxt baremetal --------

    {
        const baremetal = b.addExecutable("virtualxt", "front/baremetal/main.zig");
        baremetal.addIncludePath("front/baremetal");
        baremetal.defineCMacroRaw("ENTRY=c_main");

        // RaspberryPI3
        const bm_target = .{.cpu_arch = .aarch64, .cpu_model = .{ .explicit = &std.Target.aarch64.cpu.cortex_a53 }, .os_tag = .freestanding, .abi = .eabihf};

        baremetal.setBuildMode(mode);
        baremetal.setTarget(bm_target);
        baremetal.setOutputDir("build/baremetal");
        baremetal.setLinkerScriptPath(.{.path = "front/baremetal/link.ld"});
        baremetal.setMainPkgPath(".");

        baremetal.linkLibrary(build_libvxt(b, mode, bm_target, cpu286, cpuV20, false));
        baremetal.addIncludePath("lib/vxt/include");

        const bm_opt = c_options ++ &[_][]const u8{"-std=c11", "-pedantic"};
        baremetal.addCSourceFile("front/baremetal/main.c", bm_opt);
        baremetal.addCSourceFile("front/baremetal/uart.c", bm_opt);
        baremetal.addCSourceFile("front/baremetal/mbox.c", bm_opt);
        baremetal.addCSourceFile("front/baremetal/timer.c", bm_opt);
        baremetal.addCSourceFile("front/baremetal/video.c", bm_opt);

        baremetal.addIncludePath("lib/printf");
        baremetal.addCSourceFile("lib/printf/printf.c", bm_opt);

        // Add the vxt pirepherals directly.
        baremetal.addIncludePath("lib/vxtp");
        baremetal.addCSourceFile("lib/vxtp/disk.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/pic.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/ppi.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/pit.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/cga.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/dma.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/ctrl.c", bm_opt);
        baremetal.addCSourceFile("lib/vxtp/serial_dbg.c", bm_opt);

        const raw = b.addInstallRaw(baremetal, "../../build/baremetal/virtualxt-raspi3.img", .{.format = .bin});
        raw.step.dependOn(&baremetal.step);

        b.step("baremetal", "Build VirtualXT for bare metal target").dependOn(&raw.step);
    }

    // -------- virtualxt web --------

    {
        const wasm_target = .{.cpu_arch = .wasm32, .os_tag = .freestanding, .abi = .musl};
        const wasm = b.addSharedLibrary("virtualxt", "front/web/main.zig", .unversioned);
        wasm.addIncludePath("front/web");

        wasm.setBuildMode(mode);
        wasm.setTarget(wasm_target);
        wasm.setOutputDir("build/web");
        wasm.setMainPkgPath("."); // Needed for embedded files.

        if (cpuV20) {
            wasm.defineCMacroRaw("VXT_CPU_V20");
        }

        wasm.linkLibrary(build_libvxt(b, mode, wasm_target, false, cpuV20, false));
        wasm.addIncludePath("lib/vxt/include");
        
        const wasm_opt = opt;// ++ &[_][]const u8{"-fno-builtin"};
        wasm.addCSourceFile("front/web/main.c", wasm_opt);

        wasm.linkLibC(); // For headers only.

        wasm.addIncludePath("lib/printf");
        wasm.addCSourceFile("lib/printf/printf.c", wasm_opt);

        // Add the vxt pirepherals directly.
        wasm.addIncludePath("lib/vxtp");
        wasm.defineCMacroRaw("VXTP_CGA_BYTESWAP");
        
        wasm.addCSourceFile("lib/vxtp/disk.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/pic.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/ppi.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/pit.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/cga.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/dma.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/mouse.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/ctrl.c", wasm_opt);
        wasm.addCSourceFile("lib/vxtp/serial_dbg.c", wasm_opt);

        // https://github.com/ziglang/zig/issues/8633
        const page_size = 0x10000;
        wasm.import_memory = true; // import linear memory from the environment
        wasm.initial_memory = 350 * page_size; // initial size of the linear memory (1 page = 64kB)
        wasm.max_memory = 350 * page_size; // maximum size of the linear memory
        wasm.global_base = 6560; // offset in linear memory to place global data

        // TODO: Change this to a real copy step.
        const web = b.addSystemCommand(&[_][]const u8{"cp", "-t", "build/web/", "front/web/index.html", "front/web/script.js", "front/web/favicon.ico", "boot/freedos_web_hd.img"});
        web.step.dependOn(&wasm.step);
        b.step("web", "Build web-frontend").dependOn(&web.step);
    }

    // -------- scrambler --------

    {
        const scrambler = b.addExecutable("scrambler", null);
        scrambler.setBuildMode(mode);
        scrambler.setTarget(target);
        scrambler.setOutputDir("build/bin");
        scrambler.linkLibC();

        scrambler.linkSystemLibrary("gpiod");
        scrambler.defineCMacroRaw("PI8088");

        scrambler.linkLibrary(build_libvxt(b, mode, target, false, false, false));
        scrambler.addIncludePath("lib/vxt/include");
        scrambler.addIncludePath("lib/vxt");

        scrambler.addCSourceFile("tools/validator/pi8088/scrambler.c", opt);
        scrambler.addCSourceFile("tools/validator/pi8088/pi8088.c", opt);

        b.step("scrambler", "Build scrambler for RaspberryPi").dependOn(&scrambler.step);
    }

    // -------- test --------

    {
        const tests = b.addTest("test/test.zig");
        tests.setBuildMode(mode);
        tests.setTarget(target);

        tests.linkLibC();
        tests.defineCMacroRaw("TESTING");
        tests.defineCMacroRaw("VXT_CPU_286");
        tests.addIncludePath("test");

        const lib_vxt = build_libvxt(b, mode, target, cpu286, cpuV20, true);
        if (validator) {
            lib_vxt.linkSystemLibrary("gpiod");
            lib_vxt.defineCMacroRaw("PI8088");
            lib_vxt.addCSourceFile("tools/validator/pi8088/pi8088.c", opt);
        }

        tests.linkLibrary(lib_vxt);
        tests.addIncludePath("lib/vxt/include");
        tests.addIncludePath("lib/vxt");

        b.step("test", "Run all libvxt tests").dependOn(&tests.step);

        // -------- check --------

        {
            const check = b.addSystemCommand(&[_][]const u8{"cppcheck", "--enable=style", "lib/vxt"});
            check.step.dependOn(&tests.step);
            b.step("check", "Run CppCheck on libvxt").dependOn(&check.step);
        }
    }

    // -------- doc --------

    {
        const doc = b.addSystemCommand(&[_][]const u8{"doxygen", ".doxygen"});
        doc.cwd = "lib/vxt/include";
        b.step("doc", "Generate libvxt API documentation").dependOn(&doc.step);
    }

    // -------- run --------

    const run_cmd = exe_sdl.run();
    run_cmd.step.dependOn(&exe_sdl.step);
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    b.step("run", "Run VirtualXT").dependOn(&run_cmd.step);

    // -------- artifact --------

    b.default_step.dependOn(&exe_sdl.step);
    b.installArtifact(exe_sdl);
}
