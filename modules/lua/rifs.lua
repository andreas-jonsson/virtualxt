-- Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
--
-- This software is provided 'as-is', without any express or implied
-- warranty. In no event will the authors be held liable for any damages
-- arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose,
-- including commercial applications, and to alter it and redistribute it
-- freely, subject to the following restrictions:
--
-- 1. The origin of this software must not be misrepresented; you must not
--    claim that you wrote the original software. If you use this software
--    in a product, an acknowledgment in the product documentation would be
--    appreciated but is not required.
--
-- 2. Altered source versions must be plainly marked as such, and must not be
--    misrepresented as being the original software.
--
-- 3. This notice may not be removed or altered from any source
--    distribution.

local base_port = tonumber(({...})[1])
local registers = {0, 0, 0, 0, 0, 0, 0}
local server = {
    input_buffer = {},
    output_buffer = {}
}

function set_error(fmt, ...)
    local e = string.format(fmt, ...)
    log(e)
    server.error = e
end

function log(fmt, ...)
    print(string.format("[%s] " .. fmt, name(), ...))
end

function name()
    return "RIFS Server"
end

function io_in(port)
    if server.error then
        -- We are in a fatal error state. Wait for reset.
        return
    end

    local reg = port - base_port
    if reg == 0 then -- Serial Data Register
        if server.dlab then
            return registers[reg]
        end
        return table.remove(server.input_buffer) or 0
    elseif reg == 5 then -- Line Status Register
        local res = 0x60 -- Always assume transmition buffer is empty.
        if #server.input_buffer > 0 then
            res = res | 1
        end
        return res
    end
    return 0
end

function io_out(port, data)
    local tm = os.clock()
    local reg = port - base_port

    if reg == 0 and not server.error then -- Serial Data Register
        if server.dlab then
            return
        end

        table.insert(server.output_buffer, data)

        local sz = #server.output_buffer
        if sz >= 18 then
            if server.current_packet == nil then
                local str = string.pack(string.rep("B", sz), table.unpack(server.output_buffer))
                server.current_packet = {
                    id = string.sub(str, 1, 2),
                    length = string.unpack("H", str, 3),
                    notlength = string.unpack("H", str, 5),
                    cmd = string.unpack("H", str, 7),
                    process_id = string.unpack("H", str, 13),
                    crc32 = string.unpack("I4", str, 15)
                }
            end

            if server.current_packet.length == sz then
                if verify_packet(server.current_packet, server.output_buffer) then
                    local payload = string.pack(string.rep("B", sz - 18), select(19, table.unpack(server.output_buffer)))
                    process_packet(server.current_packet, payload)
                end
                server.output_buffer = {}
            end
        end
    elseif reg == 3 then -- Line Control Register
        local dlab = (data & 0x80) ~= 0
        if server.dlab and not dlab then
            server.error = nil
            server.input_buffer = {}
            server.output_buffer = {}
            log("Reset!")
        end
        server.dlab = dlab
    end

    wait((os.clock() - tm) * frequency())
end

function install()
    if not base_port then
        base_port = 0x178
        log("Default IO port: 0x%X", base_port)
    end
    install_io(base_port, base_port + 7)
end

function verify_packet(header, data)
    if header.length ~= (~header.notlength) & 0xFFFF then
        set_error("Invalid packet length!")
        return false
    end

    -- Reset CRC32
    data[15] = 0
    data[16] = 0
    data[17] = 0
    data[18] = 0

    local str = string.pack(string.rep("B", #data), table.unpack(data))
    if header.crc32 ~= math.crc32(str) then
        set_error("Invalid CRC!")
        return false
    end
    return true
end

function clone_header(header)
    local p = {}
    for k,v in pairs(header) do
        p[k] = v
    end
    return p
end

function process_packet(header, payload)
    local func = rifs_commands[header.cmd]
    if not func then
        log("Unknown RIFS command: 0x%X (payload size %d)", header.cmd, #payload)

        header = clone_header(header)
        header.cmd = 0x16 -- Unknown command
        server_response(header)
        return
    end

    local cmd = func(header, payload)
    if server.error then
        return
    end

    if type(cmd) == "number" then
        header = clone_header(header)
        header.cmd = 0
        server_response(header)
    end
end

function server_response(header, payload)
    if #server.input_buffer ~= 0 then
        set_error("Expected input buffer to be empty!")
        return
    end

    local payload_size = payload and #payload or 0

    header.id = "LY"
    header.length = 18 + payload_size
    header.notlength = (~header.length) & 0xFFFF

    local data = string.format("%s%s%s%s%s%s%s%s",
        header.id,
        string.pack("H", header.length),
        string.pack("H", header.notlength),
        string.pack("H", header.cmd),
        string.pack("I4", 0), -- Unused
        string.pack("H", header.process_id),
        string.pack("I4", 0), -- CRC32
        payload
    )

    server.input_buffer = {string.unpack(string.rep("B", #data), data)}

    local crc = string.pack("I4", math.crc32(data))
    server.input_buffer[15] = crc[1]
    server.input_buffer[16] = crc[2]
    server.input_buffer[17] = crc[3]
    server.input_buffer[18] = crc[4]
end

--------------------------------------------------------------------------------
-- Implementation of RIFS commands
--------------------------------------------------------------------------------

function rifs_rmdir(header, payload)
    return 0 -- Just return OK
end

rifs_commands = {
    [0x0] = rifs_rmdir
}
