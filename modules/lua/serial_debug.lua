--[[
    This is an example of a simple serial debug script.
    Add the following line to the main config file.
    
        lua=modules/lua/serial_debug.lua,0x3F8
]]

local base = tonumber(({...})[1])

function name()
    return "Serial Debug Printer"
end

function io_in(port)
    if port - base == 5 then
        return 0x20 -- Set transmission holding register empty (THRE); data can be sent.
    end
    return 0
end

function io_out(port, data)
    io.write(string.format("%c", data))
end

function install()
    if base == 0 then
        error("Invalid port address!")
    end
    install_io(base, base + 7)
end