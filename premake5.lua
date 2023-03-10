newoption {
    trigger = "sdl-path",
    value = "PATH",
    default = "PATH",
    description = "Path to SDL2 headers and libs"
}

newoption {
    trigger = "test",
    description = "Generate make files for libvxt tests"
}

newoption {
    trigger = "scrambler",
    description = "Generate make files for scrambler"
}

newoption {
    trigger = "pcap",
    description = "Enable network support by linking with pcap"
}

newoption {
    trigger = "validator",
    description = "Enable the PI8088 hardware validator"
}

newoption {
    trigger = "i286",
    description = "Provide some 286 support when running with a V20"
}

newaction {
    trigger = "check",
    description = "Run CppCheck on libvxt",
    execute = function ()
        return os.execute("cppcheck --enable=style -I lib/vxt/include lib/vxt")
    end
}

newaction {
    trigger = "doc",
    description = "Generate libvxt API documentation",
    execute = function ()
        return os.execute("cd lib/vxt/include && doxygen .doxygen")
    end
}

workspace "virtualxt"
    configurations { "release", "debug" }
    platforms { "native", "web" }
    language "C"
    cdialect "C11"

    filter "configurations:debug"
        symbols "On"

    filter "configurations:release"
        defines "NDEBUG"
        optimize "On"

    filter "options:i286"
        defines "VXT_CPU_286"

    filter "options:validator"
        defines "PI8088"
        links "gpiod"

    filter { "options:pcap", "not system:windows" }
        defines "VXTP_NETWORK"
        links "pcap"

    filter { "options:pcap", "system:windows" }
        defines { "VXTP_NETWORK", "_WINSOCK_DEPRECATED_NO_WARNINGS" }
        includedirs "tools/npcap/sdk/Include"
        links { "Ws2_32", "tools/npcap/sdk/Lib/x64/wpcap" }

    filter "system:windows"
        defines "_CRT_SECURE_NO_WARNINGS"

    filter "toolset:clang or gcc"
        buildoptions { "-pedantic", "-Wall", "-Wextra", "-Werror", "-Wno-implicit-fallthrough", "-Wno-unused-result" }

    project "inih"
        kind "StaticLib"
        files { "lib/inih/ini.h", "lib/inih/ini.c" }

    project "microui"
        kind "StaticLib"
        files { "lib/microui/src/microui.h", "lib/microui/src/microui.c" }

    project "nuked-opl3"
        kind "StaticLib"
        files { "lib/nuked-opl3/opl3.h", "lib/nuked-opl3/opl3.c" }

    project "vxt"
        kind "StaticLib"

        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        includedirs "lib/vxt/include"
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }
        
        filter "toolset:clang or gcc"
            buildoptions { "-nostdinc" }

    project "vxtp"
        kind "StaticLib"

        defines "VXTP_NUKED_OPL3"

        files { "lib/vxtp/*.h", "lib/vxtp/*.c" }
        includedirs { "lib/vxtp", "lib/vxt/include", "lib/nuked-opl3"  }

        filter "system:windows"
            includedirs "lib/dirent/include"
            buildoptions "-Wno-unused-function"

        filter "not options:pcap"
            removefiles "lib/vxtp/network.c"

        filter "toolset:gcc"
            buildoptions { "-Wno-format-truncation", "-Wno-stringop-truncation", "-Wno-stringop-overflow" }

    project "libretro-frontend"
        kind "SharedLib"
        targetname "virtualxt"
        targetdir "build/lib"
        pic "On"

        defines "VXTU_CGA_BYTESWAP"

        includedirs { "lib/vxt/include", "lib/vxtp", "lib/libretro" }
        files { "front/libretro/*.h", "front/libretro/*.c" }
        
        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

        cleancommands {
            "{RMDIR} build/lib",
            "make clean %{cfg.buildcfg}"
        }

    project "web-frontend"
        kind "ConsoleApp"
        toolset "clang"
        targetname "virtualxt"
        targetprefix ""
        targetextension ".wasm"
        targetdir "build/web"

        defines "VXTU_CGA_BYTESWAP"

        includedirs { "lib/vxt/include", "lib/vxtp", "lib/printf" }
        files { "front/web/*.h", "front/web/*.c" }

        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

        files { "lib/vxtp/ctrl.c", "lib/vxtp/serial_dbg.c" }

        files { "lib/printf/printf.h", "lib/printf/printf.c" }

        -- Perhaps move this to options?
        local page_size = 0x10000
        local memory = {
            initial = 350 * page_size,   -- Initial size of the linear memory (1 page = 64kB)
            max = 350 * page_size,       -- Maximum size of the linear memory
            base = 6560                  -- Offset in linear memory to place global data
        }

        buildoptions { "--target=wasm32", "-mbulk-memory", "-flto" }
        linkoptions { "--target=wasm32", "-nostdlib", "-Wl,--allow-undefined", "--lto-O3", ",--no-entry", ",--export-all", ",--import-memory" }
        linkoptions { "-Wl,--initial-memory=" .. tostring(memory.initial), ",--max-memory=" .. tostring(memory.max), ",--global-base=" .. tostring(memory.base) }

        postbuildcommands {
            "{COPYFILE} front/web/index.html build/web/",
            "{COPYFILE} front/web/script.js build/web/",
            "{COPYFILE} front/web/favicon.ico build/web/",
            "{COPYFILE} boot/freedos_web_hd.img build/web/",
            "{COPYDIR} lib/simple-keyboard/build/ build/web/kb/"
        }

        cleancommands {
            "{RMDIR} build/web",
            "make clean %{cfg.buildcfg}"
        }

    project "sdl2-frontend"
        kind "ConsoleApp"
        targetname "virtualxt"
        targetdir "build/bin"

        defines "VXTP_NUKED_OPL3"

        files { "front/sdl/*.h", "front/sdl/*.c" }

        links { "vxt", "vxtp", "inih", "microui", "nuked-opl3" }
        includedirs { "lib/vxt/include", "lib/vxtp", "lib/inih", "lib/microui/src", "lib/nuked-opl3" }

        cleancommands {
            "{RMDIR} build/bin",
            "make clean %{cfg.buildcfg}"
        }

        filter "options:validator"
            files { "tools/validator/pi8088/pi8088.c", "tools/validator/pi8088/udmask.h" }

        filter "not options:sdl-path=PATH"
            local sdl = _OPTIONS["sdl-path"]
            includedirs { sdl .. "/include", sdl .. "/include/SDL2" }
            if os.target() == "windows" then
                local ty = "/lib/x64"
                if string.contains(sdl, "mingw") then
                    ty = "/lib"
                end
                links { sdl .. ty .. "/SDL2main" }
                links { sdl .. ty .. "/SDL2" }
            else
                links { sdl ..  "/lib/SDL2" }
            end
        
        filter "options:sdl-path=PATH"
            if os.target() == "windows" then
                links "SDL2main"
            end
            links "SDL2"

        filter { "system:windows", "options:pcap" }
            targetname "virtualxt-net"

        filter "system:windows"
            linkoptions "-Xlinker /subsystem:windows"
            links { "Shlwapi", "Shell32" }

        filter "toolset:clang or gcc"
            buildoptions "-Wno-unused-parameter"

        filter "toolset:clang"
            buildoptions { "-Wno-missing-field-initializers", "-Wno-missing-braces" }

        filter "toolset:gcc"
            buildoptions "-Wno-maybe-uninitialized"

if _OPTIONS["scrambler"] then
    project "scrambler"
        kind "ConsoleApp"
        targetname "scrambler"
        targetdir "build/bin"
        links "gpiod"
        defines { "PI8088", "VXT_CPU_286" }

        files { "tools/validator/pi8088/scrambler.c", "tools/validator/pi8088/pi8088.c", "tools/validator/pi8088/udmask.h" }
        
        includedirs "lib/vxt/include"
        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

        cleancommands {
            "{RMDIR} build/bin",
            "make clean %{cfg.buildcfg}"
        }
end

if _OPTIONS["test"] then
    project "test"
        kind "ConsoleApp"
        targetdir "test"
        includedirs "lib/vxt/include"
        defines { "TESTING", "VXT_CPU_286" }
        files { "test/test.c", "lib/vxt/**.h", "lib/vxt/*.c" }
        optimize "Off"
        symbols "On"

        postbuildcommands "./test/test"
        cleancommands "{RMDIR} test"

        filter { "toolset:clang or gcc" }
            buildoptions { "-Wno-unused-function", "-Wno-unused-variable", "--coverage" }
            linkoptions "--coverage"
    
    io.writefile("test/test.c", (function()
        local test_names = {}
        for _,file in pairs(os.matchfiles("lib/vxt/*.c")) do
            for line in io.lines(file) do
                if string.startswith(line, "TEST(") then
                    table.insert(test_names, string.sub(line, 6, -2))
                end
            end
        end

        local head = '#include <stdio.h>\n#include "../lib/vxt/testing.h"\n\n'
        head = head .. '#define RUN_TEST(t) { ok += run_test(t) ? 1 : 0; num++; }\n\n'
        local body = "\t(void)argc; (void)argv;\n\tint ok = 0, num = 0;\n\n"
        for _,name in ipairs(test_names) do
            head = string.format("%sextern int test_%s(struct Test T);\n", head, name)
            body = string.format("%s\tRUN_TEST(test_%s);\n", body, name)
        end
        body = string.format('%s\n\tprintf("%%d/%%d tests passed!\\n", ok, num);\n\treturn (num - ok) ? -1 : 0;\n', body)
        return string.format("%s\nint main(int argc, char *argv[]) {\n%s}\n", head, body)
    end)())
end