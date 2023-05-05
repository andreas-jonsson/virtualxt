files "network.c"
links "pcap"

filter "system:windows"
    includedirs "../../tools/npcap/sdk/Include"
    defines { "_CRT_SECURE_NO_WARNINGS", "_WINSOCK_DEPRECATED_NO_WARNINGS" }
    links "Ws2_32"