newoption {
    trigger = "sdl-config",
    value = "PATH",
    description = "Path to sdl2-config script"
}

newoption {
    trigger = "test",
    description = "Generate make files for libvxt tests"
}

newoption {
    trigger = "modules",
    description = "Generate make files for all modules"
}

newoption {
    trigger = "static",
    description = "Use static modules"
}

newoption {
    trigger = "memclear",
    description = "Clear main memory when initializing"
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
        return os.execute("cppcheck --force --enable=style -I lib/vxt/include lib/vxt")
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

    local handle = io.popen("git branch --show-current")
    if handle then
        local branch = handle:read("l") or "?"
        print("Branch: " .. branch)
        if branch == "release" then
            defines "VXT_VERSION_RELEASE"
        end
        handle:close()
    end

    filter "configurations:debug"
        symbols "On"
        optimize "Off"
        defines "VXT_DEBUG_PREFETCH"

    filter "configurations:release"
        defines "NDEBUG"
        optimize "On"

    filter "platforms:web"
        toolset "clang"
        defines "VXT_NO_LIBC"
        includedirs "lib/libc"
        buildoptions { "--target=wasm32", "-mbulk-memory", "-flto" }

    filter "options:memclear"
        defines "VXTU_MEMCLEAR"

    filter "options:i286"
        defines "VXT_CPU_286"

    filter "options:validator"
        defines "PI8088"
        links "gpiod"

    filter "system:windows"
        defines "_CRT_SECURE_NO_WARNINGS"
        
    filter "action:vs*"
        architecture "x86_64"

    filter { "toolset:clang or gcc", "not action:vs*" }
        buildoptions { "-pedantic", "-Wall", "-Wextra", "-Werror", "-Wno-implicit-fallthrough", "-Wno-unused-result" }

    filter { "toolset:clang or gcc", "configurations:debug" }
        buildoptions "-Wno-error"
        sanitize { "Address", "Fuzzer" }

    local modules = {}
    local module_opt = _OPTIONS["modules"]

    if module_opt then
        filter {}

        defines "VXTU_MODULES"
        if _OPTIONS["static"] then
            defines "VXTU_STATIC_MODULES"
        end

        local mod_list = {}
        local inclusive = true;
 
        if string.sub(module_opt, 1, 1) == "-" then
            inclusive = false
            module_opt = string.sub(module_opt, 2)
        end

        for _,f in ipairs(os.matchfiles("modules/**/premake5.lua")) do
            local enable = module_opt == "" or not inclusive
            local name = path.getname(path.getdirectory(f))

            for mod in string.gmatch(module_opt, "[%w%-]+") do
                if mod == name then
                    enable = inclusive
                    break
                end
            end
            if enable then
                table.insert(mod_list, name)
                defines { "VXTU_MODULE_" .. string.upper(name) }
            end
        end

        for _,name in ipairs(mod_list) do
            table.insert(modules, name)

            project(name)
                if _OPTIONS["static"] then
                    kind "StaticLib"
                    basedir "."
                else
                    kind "SharedLib"
                    targetdir "modules"
                    targetprefix ""
                    targetextension ".vxt"
                    basedir "."
                    links "vxt"
                    pic "On"
                end

                includedirs { "lib/vxt/include", "front/common" }
                defines { "VXTU_MODULE_NAME=" .. name }

                cleancommands {
                    "{RMFILE} modules/" .. name .. ".*",
                    "make clean %{cfg.buildcfg}"
                }

                filter {}
    
            dofile("modules/" .. name .. "/premake5.lua")
        end
    end

    -- This is just a dummy project.
    project "modules"
        kind "StaticLib"
        files "modules/dummy.c"
        links(modules)
        io.writefile("modules/dummy.c", "int dummy;\n")

    project "inih"
        kind "StaticLib"
        files { "lib/inih/ini.h", "lib/inih/ini.c" }

    project "microui"
        kind "StaticLib"
        files { "lib/microui/src/microui.h", "lib/microui/src/microui.c" }

    project "miniz"
        kind "StaticLib"
        pic "On"
        files { "lib/miniz/miniz.h", "lib/miniz/miniz.c" }
        
        filter "toolset:clang or gcc"
            buildoptions "-Wno-error=type-limits"

    project "fat16"
        kind "StaticLib"
        pic "On"
        files {
            "lib/fat16/fat16.h",
            "lib/fat16/blockdev.h",
            "lib/fat16/fat16_internal.h",
            "lib/fat16/fat16.c"
        }

    project "vxt"
        if _OPTIONS["modules"] and not _OPTIONS["static"] then
            kind "SharedLib"
            targetdir "build/bin"
            pic "On"
        else
            kind "StaticLib"
        end

        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        includedirs "lib/vxt/include"
        defines "VXT_EXPORT"
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

    project "libretro-frontend"
        kind "SharedLib"
        targetname "virtualxt_libretro"
        targetprefix ""
        targetdir "build/lib"
        pic "On"

        includedirs { "lib/libretro", "front/common" }
        files { "front/libretro/*.h", "front/libretro/*.c" }
        
        defines { "VXTU_CGA_RED=2", "VXTU_CGA_GREEN=1", "VXTU_CGA_BLUE=0", "VXTU_CGA_ALPHA=3" }
        includedirs "lib/vxt/include"
        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

        links { "miniz", "fat16" }
        includedirs { "lib/miniz", "lib/fat16" }
        defines "ZIP2IMG"

        cleancommands {
            "{RMDIR} build/lib",
            "make clean %{cfg.buildcfg}"
        }

        filter "toolset:clang or gcc"
            buildoptions { "-Wno-atomic-alignment", "-Wno-deprecated-declarations" }

        -- TODO: Remove this filter! This is here to fix an issue with the GitHub builder.
        filter "not system:windows"
            postbuildcommands "{COPYFILE} front/libretro/virtualxt_libretro.info build/lib/"

    project "web-frontend"
        kind "ConsoleApp"
        targetname "virtualxt"
        targetprefix ""
        targetextension ".wasm"
        targetdir "build/web"

        includedirs "front/common"
        files { "front/web/*.h", "front/web/*.c" }

        includedirs "lib/vxt/include"
        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }

        includedirs "lib/libc"
        defines { "SCANF_FREESTANDING", "SCANF_DISABLE_SUPPORT_FLOAT" }
        files { "lib/libc/*.h", "lib/libc/*.c", "lib/scanf/scanf.c", "lib/printf/printf.h", "lib/printf/printf.c" }

        files "modules/modules.h"
        includedirs "modules"

        if _OPTIONS["modules"] and _OPTIONS["static"] then
            links(modules)
        end

        -- Perhaps move this to options?
        local page_size = 0x10000
        local memory = {
            initial = 350 * page_size,   -- Initial size of the linear memory (1 page = 64kB)
            max = 350 * page_size,       -- Maximum size of the linear memory
            base = 6560                  -- Offset in linear memory to place global data
        }

        linkoptions { "--target=wasm32", "-nostdlib", "-Wl,--allow-undefined", "-Wl,--lto-O3", "-Wl,--no-entry", "-Wl,--export-all", "-Wl,--import-memory" }
        linkoptions { "-Wl,--initial-memory=" .. tostring(memory.initial), "-Wl,--max-memory=" .. tostring(memory.max), "-Wl,--global-base=" .. tostring(memory.base) }

        postbuildcommands {
            "{COPYFILE} front/web/index.html build/web/",
            "{COPYFILE} front/web/script.js build/web/",
            "{COPYFILE} front/web/favicon.ico build/web/",
            "{COPYFILE} front/web/disk.png build/web/",
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
        
        files "modules/modules.h"
        includedirs "modules"

        if _OPTIONS["modules"] then
            if _OPTIONS["static"] then
                links(modules)
            else
                dependson "modules"
            end
        end

        files { "front/sdl/*.h", "front/sdl/*.c" }
        includedirs { "lib/vxt/include", "lib/inih", "lib/microui/src", "front/common" }
        links { "vxt", "inih", "microui" }

        cleancommands {
            "{RMDIR} build/bin",
            "make clean %{cfg.buildcfg}"
        }

        filter "action:vs*"
            libdirs { path.join(_OPTIONS["sdl-config"], "lib", "x64") }
            includedirs { path.join(_OPTIONS["sdl-config"], "include") }
            links { "SDL2", "SDL2main" }

        filter "not action:vs*"
            local sdl_cfg = path.join(_OPTIONS["sdl-config"], "sdl2-config")
            buildoptions { string.format("`%s --cflags`", sdl_cfg) }
            linkoptions { string.format("`%s --libs`", sdl_cfg) }

        filter "options:validator"
            files { "tools/validator/pi8088/pi8088.c", "tools/validator/pi8088/udmask.h" }

        filter "toolset:clang or gcc"
            buildoptions "-Wno-unused-parameter"
            linkoptions "-Wl,-rpath='$$ORIGIN'/../lib"

        filter "toolset:clang"
            buildoptions { "-Wno-missing-field-initializers", "-Wno-missing-braces" }

        filter "toolset:gcc"
            buildoptions "-Wno-maybe-uninitialized"

if _OPTIONS["test"] then
    project "test"
        kind "ConsoleApp"
        targetdir "test"
        includedirs "lib/vxt/include"
        defines { "TESTING", "VXTU_MEMCLEAR" }
        files { "test/test.c", "lib/vxt/**.h", "lib/vxt/*.c" }

        optimize "Off"
        symbols "On"
        sanitize { "Address", "Fuzzer" }

        postbuildcommands "./test/test"
        cleancommands "{RMDIR} test"

        filter { "toolset:clang or gcc" }
            buildoptions { "-Wno-unused-function", "-Wno-unused-variable", "--coverage" }
            linkoptions "--coverage"
    
    io.writefile("test/test.c", (function()
        print("Searching for tests:")

        local pattern = _OPTIONS["test"]
        local externals = ""
        local calls = ""

        for _,file in pairs(os.matchfiles("lib/vxt/**.c")) do
            if not pattern or string.find(file, pattern, 1, true) then
                print(file)
                files { file } -- Ensure any files in recursive directories are added.
                for line in io.lines(file) do
                    if string.startswith(line, "TEST(") then
                        local name = string.sub(line, 6, -2)
                        externals = externals .. string.format("extern int test_%s(struct Test T);\n", name)
                        calls = calls .. string.format("\tRUN_TEST(test_%s);\n", name)
                    end
                end
            end
        end

        print("Generating test code:")

        -- Avoid using string.format here as the strings can be very large.

        local head = '#include <stdio.h>\n#include "../lib/vxt/testing.h"\n\n#define RUN_TEST(t) { ok += run_test(t) ? 1 : 0; num++; }\n\n' .. externals
        local body = "\t(void)argc; (void)argv;\n\tint ok = 0, num = 0;\n\n" .. calls

        body = body .. '\n\tprintf("%d/%d tests passed!\\n", ok, num);\n\treturn (num - ok) ? -1 : 0;\n'
        return head .. "\nint main(int argc, char *argv[]) {\n" .. body .. "}\n"
    end)())
end

io.writefile("modules/modules.h", (function()
    local is_static = _OPTIONS["static"]
    local str = "#include <vxt/vxtu.h>\n\nstruct vxtu_module_entry {\n\tconst char *name;\n\tvxtu_module_entry_func *(*entry)(int(*)(const char*, ...));\n};\n\n"
    if is_static then
        for _,mod in ipairs(modules) do
            str = string.format("%s#ifdef VXTU_MODULE_%s\n\textern vxtu_module_entry_func *_vxtu_module_%s_entry(int(*)(const char*, ...));\n#endif\n", str, string.upper(mod), mod)
        end
        str = str .. "\n"
    end
    str = string.format("%sconst struct vxtu_module_entry vxtu_module_table[] = {\n", str)
    for _,mod in ipairs(modules) do
        if is_static then
            str = string.format('%s\t#ifdef VXTU_MODULE_%s\n\t\t{ "%s", _vxtu_module_%s_entry },\n\t#endif\n', str, string.upper(mod), mod, mod)
        else
            str = string.format('%s\t{ "%s", NULL },\n', str, mod)  
        end
    end
    return str .. "\t{ NULL, NULL }\n};\n"
end)())