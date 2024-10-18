files "ebridge.c"
filter "system:windows"
	defines "_WINSOCK_DEPRECATED_NO_WARNINGS"
	links "Ws2_32"
