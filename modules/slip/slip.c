// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxtu.h>

#ifdef __linux__

#include <assert.h>
#include <stdio.h>

#include <arpa/inet.h>

#define BUFFER_MAX_SIZE 0x10000

struct slip {
	vxt_word base_port;
    struct vxt_pirepheral *uart;
    
    int packet_size;
    vxt_byte buffer[BUFFER_MAX_SIZE];
    bool has_esc, is_compleat;
};

static uint16_t ip_header_checksum(uint16_t *header) {
	int size = (*header & 0xF) * 4;
	uint32_t sum = 0;

	while (size > 1) {
		sum += *header++;
		size -= 2;
	}
	
	//if (size > 0)
	//	sum += *header & htons(0xFF00);
	assert(!size);
		
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	sum = ~sum;
	return (uint16_t)sum;
}

static void new_packet(struct slip *c) {
	c->packet_size = 0;
	c->is_compleat = c->has_esc = false;
}

static void uart_config_cb(struct vxt_pirepheral *uart, const struct vxtu_uart_registers *regs, int idx, void *udata) {
	(void)uart; (void)regs;
	(void)idx; (void)udata;
}

static void process_tcp_packet(struct slip *c, vxt_byte *packet) {
	(void)c; (void)packet;
	VXT_LOG("Process TCP packet!");
}

static void process_ip_packet(struct slip *c) {
	int size = ntohs(((uint16_t*)c->buffer)[1]);
	if (size > c->packet_size) {
		VXT_LOG("IP packet is incompleat!");
		return;
	}
	c->packet_size = size; // SLIP driver seems to add junk at the end of packages?
	
	#define CHECKSUM (((uint16_t*)c->buffer)[5])
	uint16_t checksum = CHECKSUM;
	CHECKSUM = 0;

	if (ip_header_checksum((uint16_t*)c->buffer) != checksum) {
		VXT_LOG("IP packet checksum error!");
		return;
	}
	
	CHECKSUM = checksum;
	#undef CHECKSUM
	
	int version = c->buffer[0] >> 4;
	if (version != 4) {
		VXT_LOG("Invalid IP packet version: %d", version);
		return;
	}
	
	int offset = (*c->buffer & 0xF) * 4;
	int protocol = c->buffer[9];
	
	switch (protocol) {
		case 0x6: // TCP
			process_tcp_packet(c, &c->buffer[offset]);
			break;
		case 0x11: // UDP
		default:
			VXT_LOG("Unsupported protocol: 0x%x", protocol);
			return;
	}
	
	vxt_byte *ip = &c->buffer[12];
	VXT_LOG("Packet source: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	ip = &c->buffer[12 + 4];
	VXT_LOG("Packet destination: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static void push_data(struct slip *c, vxt_byte data) {
	if (c->packet_size >= BUFFER_MAX_SIZE) {
		VXT_LOG("SLIP buffer overflow!");
		new_packet(c);
		return;
	}
	c->buffer[c->packet_size++] = data;
}

static void uart_data_cb(struct vxt_pirepheral *uart, vxt_byte data, void *udata) {
	struct slip *c = (struct slip*)udata; (void)uart;
	
	if (c->has_esc) {
		c->has_esc = false;
		switch (data) {
			case 0xDC: // ESC_END
				push_data(c, 0xC0);
				break;
			case 0xDD: // ESC_ESC
				push_data(c, 0xDB);
				break;
			default:
				VXT_LOG("Corrupt SLIP packet!");
				new_packet(c);
		}
	} else if (data == 0xDB) { // ESC
		c->has_esc = true;
	} else if (data == 0xC0) { // END
		if (c->is_compleat && c->packet_size)
			process_ip_packet(c);
		new_packet(c);
		c->is_compleat = true;
	} else if (c->is_compleat) {
		push_data(c, data);
	}
}

static vxt_error timer(struct slip *c, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    (void)c;
	return VXT_NO_ERROR;
}

static vxt_error install(struct slip *c, vxt_system *s) {
    for (int i = 0; i < VXT_MAX_PIREPHERALS; i++) {
        struct vxt_pirepheral *ip = vxt_system_pirepheral(s, (vxt_byte)i);
        if (ip && (vxt_pirepheral_class(ip) == VXT_PCLASS_UART) && (vxtu_uart_address(ip) == c->base_port)) {
            struct vxtu_uart_interface intrf = {
                .config = &uart_config_cb,
				.data = &uart_data_cb,
                .ready = NULL,
                .udata = (void*)c
            };
            vxtu_uart_set_callbacks(ip, &intrf);
            c->uart = ip;
            break;
        }
    }
    if (!c->uart) {
        VXT_LOG("Could not find UART with base address: 0x%X", c->base_port);
        return VXT_USER_ERROR(0);
    }

	vxt_system_install_timer(s, VXT_GET_PIREPHERAL(c), 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct slip *c) {
	new_packet(c);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct slip *c) {
    vxt_system_allocator(VXT_GET_SYSTEM(c))(VXT_GET_PIREPHERAL(c), 0);
    return VXT_NO_ERROR;
}

static const char *name(struct slip *c) {
    (void)c; return "SLIP";
}

VXTU_MODULE_CREATE(slip, {
    if (sscanf(ARGS, "%hx", &DEVICE->base_port) != 1) {
		VXT_LOG("Invalid UART address: %s", ARGS);
		return NULL;
    }

    PIREPHERAL->install = &install;
	PIREPHERAL->destroy = &destroy;
	PIREPHERAL->timer = &timer;
	PIREPHERAL->reset = &reset;
    PIREPHERAL->name = &name;
})

#else

struct slip { int _; };
VXTU_MODULE_CREATE(slip, { return NULL; })

#endif
