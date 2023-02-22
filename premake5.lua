newoption {
    trigger = "sdl-path",
    value = "PATH",
    description = "Path to SDL2 headers and libs"
}

workspace "virtualxt"
    configurations { "Release", "Debug" }
    location "premake"
    language "C"
    cdialect "C11"

    filter { "action:gmake" }
        buildoptions { "-pedantic", "-Wall", "-Wextra", "-Werror", "-Wno-implicit-fallthrough", "-Wno-unused-result" }

    project "inih"
        kind "StaticLib"
        defines "NDEBUG"
        optimize "On"
        files { "lib/inih/ini.h", "lib/inih/ini.c" }

    project "microui"
        kind "StaticLib"
        defines "NDEBUG"
        optimize "On"
        files { "lib/microui/src/microui.h", "lib/microui/src/microui.c" }

    project "nuked-opl3"
        kind "StaticLib"
        defines "NDEBUG"
        optimize "On"
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

        filter "action:gmake"
            buildoptions { "-Wno-format-truncation", "-Wno-stringop-truncation", "-Wno-stringop-overflow" }

        filter "configurations:Debug"
            defines "DEBUG"
            symbols "On"

        filter "configurations:Release"
            defines "NDEBUG"
            optimize "On"

    project "sdl2-frontend"
        kind "ConsoleApp"
        targetname "virtualxt"
        targetdir "build/bin"

        files { "front/sdl/*.h", "front/sdl/*.c" }

        links { "SDL2", "libvxt", "libvxtp", "inih", "microui", "nuked-opl3" }
        includedirs { "lib/vxt/include", "lib/vxtp", "lib/inih", "lib/microui/src" }

        filter "options:sdl-path"
            libdirs { _OPTIONS["sdl-path"] }

        filter "configurations:Debug"
            defines "DEBUG"
            symbols "On"

        filter "configurations:Release"
            defines "NDEBUG"
            optimize "On"

        filter "action:gmake"
            buildoptions { "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-maybe-uninitialized", "-Wno-stringop-truncation" }