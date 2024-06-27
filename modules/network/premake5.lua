newoption {
    trigger = "pcap",
    description = "Link with libpcap for network support"
}

files "network.c"

if _OPTIONS["pcap"] then
    defines { "LIBPCAP", "_DEFAULT_SOURCE=1" }

	filter "system:windows"
	    includedirs "../../tools/npcap/sdk/Include"
	    defines "_WINSOCK_DEPRECATED_NO_WARNINGS"

	module_link_callback(function()
		filter "not system:windows"
			links "pcap"

		filter "system:windows"
			links { "Ws2_32", "../../tools/npcap/sdk/Lib/x64/wpcap" }
	end)
end
