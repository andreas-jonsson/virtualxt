files "network.c"

filter "system:windows"
    includedirs "../../tools/npcap/sdk/Include"
    defines "_WINSOCK_DEPRECATED_NO_WARNINGS"

module_link_callback(function()
	filter "not system:windows"
		links "pcap"

	filter "system:windows"
		links { "Ws2_32", "../../tools/npcap/sdk/Lib/x64/wpcap" }
end)
