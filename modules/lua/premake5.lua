files {
    "lua-5.4.6/src/*.c",
    "lua-5.4.6/src/*.h"
}

removefiles {
    "lua-5.4.6/src/lmain.c",
    "lua-5.4.6/src/lua.c"
}

includedirs "lua-5.4.6/src"
files "lua.c"
buildoptions "-Wno-pedantic"