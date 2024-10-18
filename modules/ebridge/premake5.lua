files "ebridge.c"
filter "system:windows"
	defines "_WINSOCK_DEPRECATED_NO_WARNINGS"
	
module_link_callback(function()
	filter "system:windows"
		links "Ws2_32"
end)
