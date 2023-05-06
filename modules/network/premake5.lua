files "network.c"

filter "not system:windows"
    links "pcap"

filter "system:windows"
    includedirs "../../tools/npcap/sdk/Include"
    defines "_WINSOCK_DEPRECATED_NO_WARNINGS"
    links { "Ws2_32", "tools/npcap/sdk/Lib/x64/wpcap" }