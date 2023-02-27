newoption {
    trigger = "sdl-path",
    value = "PATH",
    default = "PATH",
    description = "Path to SDL2 headers and libs"
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
    description = "Select some 286 support when running with a V20"
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
    location "premake"
    language "C"
    cdialect "C11"

    filter "configurations:debug"
        defines "DEBUG"
        symbols "On"

    filter "configurations:release"
        defines "NDEBUG"
        optimize "On"

    filter "options:i286"
        defines "VXT_CPU_286"

    filter "options:validator"
        defines "PI8088"
        links "gpiod"

    filter "options:pcap"
        defines "VXTP_NETWORK"
        links "pcap"

    filter { "action:gmake" }
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

    project "libvxt"
        kind "StaticLib"

        files { "lib/vxt/**.h", "lib/vxt/*.c" }
        removefiles { "lib/vxt/testing.h", "lib/vxt/testsuit.c" }
        includedirs { "lib/vxt/include" }

        filter "action:gmake"
            buildoptions { "-nostdinc", "-Wno-unused-function", "-Wno-unused-variable" }

        filter "configurations:Debug"
            defines "DEBUG"
            symbols "On"

        filter "configurations:Release"
            defines "NDEBUG"
            optimize "On"

    project "libvxtp"
        kind "StaticLib"

        files { "lib/vxtp/*.h", "lib/vxtp/*.c" }
        includedirs { "ib/vxtp", "lib/vxt/include", "lib/nuked-opl3"  }

        filter "not options:pcap"
            removefiles "lib/vxtp/network.c"

        filter "action:gmake"
            buildoptions { "-Wno-format-truncation", "-Wno-stringop-truncation", "-Wno-stringop-overflow" }

    project "sdl2-frontend"
        kind "ConsoleApp"
        targetname "virtualxt"
        targetdir "build/bin"

        files { "front/sdl/*.h", "front/sdl/*.c" }

        links { "libvxt", "libvxtp", "inih", "microui", "nuked-opl3" }
        includedirs { "lib/vxt/include", "lib/vxtp", "lib/inih", "lib/microui/src" }

        filter "options:validator"
            files "tools/validator/pi8088/pi8088.c"

        filter "not options:sdl-path=PATH"
            includedirs { _OPTIONS["sdl-path"] .. "/include" }
            links { _OPTIONS["sdl-path"] .. "/lib/SDL2" }
        
        filter "options:sdl-path=PATH"
            links "SDL2"

        filter "action:gmake"
            buildoptions { "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-maybe-uninitialized", "-Wno-stringop-truncation" }