files "network.c"

filter "toolset:clang or gcc"
    buildoptions "-Wno-pedantic"

filter "system:windows"
    includedirs "tools/npcap/sdk/Include"