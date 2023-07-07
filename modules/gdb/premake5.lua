defines "GDBSTUB_IMPLEMENTATION"
files "gdb.c"

filter "toolset:clang or gcc"
    buildoptions { "-Wno-unused-function", "-Wno-unused-parameter" }

filter "system:windows"
    defines "_WINSOCK_DEPRECATED_NO_WARNINGS"
    links "Ws2_32"
