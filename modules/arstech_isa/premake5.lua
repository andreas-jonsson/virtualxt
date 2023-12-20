newoption {
    trigger = "arstech",
    description = "Link with Arstech USB library (arsusb4)"
}

files "isa.c"

if _OPTIONS["arstech"] then
	defines "ARSTECHUSB"
	module_link_callback(function()
			links(_OPTIONS["arstech"])
	end)
end
