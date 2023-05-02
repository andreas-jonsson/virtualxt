defines { "_POSIX_C_SOURCE=200809L", "FASYNC=O_ASYNC" }
files { "isa.c", "ch36x/ch36x_lib.h", "ch36x/ch36x_lib.c" }
buildoptions { "-Wno-unused-parameter", "-Wno-unused-variable", "-Wno-type-limits" }