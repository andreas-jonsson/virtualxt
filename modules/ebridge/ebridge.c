// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

#include <vxt/vxtu.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif

#define MAX_PACKET_SIZE 0xFFF0	// Should match server MTU.
#define POLL_DELAY 1000			// Poll every millisecond.
#define DEFAULT_PORT 1235

const vxt_byte default_mac[6] = { 0x00, 0x0B, 0xAD, 0xC0, 0xFF, 0xEE };

enum pkt_driver_error {
	BAD_HANDLE = 1,		// Invalid handle number.
	NO_CLASS,			// No interfaces of specified class found.
	NO_TYPE,			// No interfaces of specified type found.
	NO_NUMBER,			// No interfaces of specified number found.
	BAD_TYPE,			// Bad packet type specified.
	NO_MULTICAST,		// This interface does not support multicast.
	CANT_TERMINATE,		// This packet driver cannot terminate.
	BAD_MODE,			// An invalid receiver mode was specified.
	NO_SPACE,			// Operation failed because of insufficient space.
	TYPE_INUSE,			// The type had previously been accessed, and not released.
	BAD_COMMAND,		// The command was out of range, or not implemented.
	CANT_SEND,			// The packet couldn't be sent (usually hardware error).
	CANT_SET,			// Hardware address couldn't be changed (more than 1 handle open).
	BAD_ADDRESS,		// Hardware address has bad length or format.
	CANT_RESET			// Couldn't reset interface (more than 1 handle open).
};

enum pkt_driver_command {
	DRIVE_INFO = 1,
	ACCESS_TYPE,
	RELEASE_TYPE,
	SEND_PACKET,
	TERMINATE,
	GET_ADDRESS,
	RESET_INTERFACE
};

struct ebridge {
	bool can_recv;
	
	vxt_word rx_port;
	vxt_word tx_port;
	
	int sockfd;
	struct sockaddr_in addr;
	char bridge_addr[64];
	
	vxt_byte mac_addr[6];

	// Temp buffer for package
	vxt_byte buffer[MAX_PACKET_SIZE];
	
	vxt_word rx_len;
	vxt_byte rx_buffer[MAX_PACKET_SIZE];
	
	vxt_word cb_seg;
	vxt_word cb_offset;
};

static const char *command_to_string(int cmd) {
	#define CASE(cmd) case cmd: return #cmd;
	switch (cmd) {
		CASE(DRIVE_INFO)
		CASE(ACCESS_TYPE)
		CASE(RELEASE_TYPE)
		CASE(SEND_PACKET)
		CASE(TERMINATE)
		CASE(GET_ADDRESS)
		CASE(RESET_INTERFACE)
		case 0xFE: return "GET_CALLBACK";
		case 0xFF: return "COPY_PACKAGE";
	}
	#undef CASE
	return "UNSUPPORTED COMMAND";
}

static vxt_byte in(struct ebridge *n, vxt_word port) {
	(void)n; (void)port;
	return 0; // Return 0 to indicate that we have a network card.
}

static void out(struct ebridge *n, vxt_word port, vxt_byte data) {
	(void)port; (void)data;
	vxt_system *s = VXT_GET_SYSTEM(n);
	struct vxt_registers *r = vxt_system_registers(s);

	// Assume no error
	r->flags &= ~VXT_CARRY;
	
	#define ENSURE_HANDLE assert(r->bx == 0)
	#define LOG_COMMAND VXT_LOG(command_to_string(r->ah));

	switch (r->ah) {
		case DRIVE_INFO: LOG_COMMAND
			r->bx = 1; // version
			r->ch = 1; // class
			r->dx = 1; // type
			r->cl = 0; // number
			r->al = 1; // functionality
			// Name in DS:SI is filled in by the driver.
			break;
		case ACCESS_TYPE: LOG_COMMAND
			assert(r->cx == 0);

			// We only support capturing all types.
			// typelen != any_type
			if (r->cx != 0) {
				r->flags |= VXT_CARRY;
				r->dh = BAD_TYPE;
				return;
			}

			n->can_recv = true;
			n->cb_seg = r->es;
			n->cb_offset = r->di;

			VXT_LOG("Callback address: %X:%X", r->es, r->di);

			r->ax = 0; // Handle
			break;
		case RELEASE_TYPE: LOG_COMMAND
			ENSURE_HANDLE;
			break;
		case TERMINATE: LOG_COMMAND
			ENSURE_HANDLE;
			r->flags |= VXT_CARRY;
			r->dh = CANT_TERMINATE;
			return;
		case SEND_PACKET:
			if (r->cx > MAX_PACKET_SIZE) {
				VXT_LOG("Can't send! Invalid package size!");
				r->flags |= VXT_CARRY;
				r->dh = CANT_SEND;
				return;
			}

			for (int i = 0; i < (int)r->cx; i++)
				n->buffer[i] = vxt_system_read_byte(s, VXT_POINTER(r->ds, r->si + i));

			if (sendto(n->sockfd, (void*)n->buffer, r->cx, 0, (const struct sockaddr*)&n->addr, sizeof(n->addr)) != r->cx)
				VXT_LOG("Could not send packet!");
				
			VXT_LOG("Sent package with size: %d bytes", r->cx);
			break;
		case GET_ADDRESS: LOG_COMMAND
			ENSURE_HANDLE;
			if (r->cx < 6) {
				VXT_LOG("Can't fit address!");
				r->flags |= VXT_CARRY;
				r->dh = NO_SPACE;
				return;
			}

			r->cx = 6;
			for (int i = 0; i < (int)r->cx; i++)
				vxt_system_write_byte(s, VXT_POINTER(r->es, r->di + i), n->mac_addr[i]);
			break;
		case RESET_INTERFACE: LOG_COMMAND
			ENSURE_HANDLE;
			VXT_LOG("Reset interface!");
			n->can_recv = false; // Not sure about this...
			n->rx_len = 0;
			break;
		case 0xFE: // get_callback
			r->es = n->cb_seg;
			r->di = n->cb_offset;
			r->bx = 0; // Handle
			r->cx = (vxt_word)n->rx_len;
			break;
		case 0xFF: // copy_package
			// Do we have a valid buffer?
			if (r->es || r->di) {
				for (int i = 0; i < n->rx_len; i++)
					vxt_system_write_byte(s, VXT_POINTER(r->es, r->di + i), n->rx_buffer[i]);

				// Callback expects buffer in DS:SI
				r->ds = r->es;
				r->si = r->di;
				r->cx = (vxt_word)n->rx_len;
				
				VXT_LOG("Received package with size: %d bytes", n->rx_len);
			} else {
				VXT_LOG("Package discarded by driver!");
			}

			n->rx_len = 0;
			n->can_recv = true;
			break;
		default: LOG_COMMAND
			r->flags |= VXT_CARRY;
			r->dh = BAD_COMMAND;
			return;
	}
}

static vxt_error reset(struct ebridge *n, struct ebridge *state) {
	if (state)
		return VXT_CANT_RESTORE;
		
	n->rx_len = n->cb_seg = n->cb_offset = 0;
	n->can_recv = false;
	return VXT_NO_ERROR;
}

static const char *name(struct ebridge *n) {
	(void)n; return "Network Adapter";
}

static bool has_data(int fd) {
	if (fd == -1)
		return false;

	fd_set fds;
	FD_ZERO(&fds);
    FD_SET(fd, &fds);
	struct timeval timeout = {0};

    return select(fd + 1, &fds, NULL, NULL, &timeout) > 0;
}

static vxt_error timer(struct ebridge *n, vxt_timer_id id, int cycles) {
	(void)id; (void)cycles;
	
	if (!n->can_recv || !has_data(n->sockfd))
		return VXT_NO_ERROR;

	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	
	ssize_t sz = recvfrom(n->sockfd, (void*)n->rx_buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
	if (sz <= 0) {
		VXT_LOG("'recvfrom' failed!");
		return VXT_NO_ERROR;
	}
	
	// This should be done by the bridge but we need to be sure.
	if (memcmp(n->rx_buffer, n->mac_addr, sizeof(n->mac_addr)))
		return VXT_NO_ERROR;

	n->can_recv = false;
	n->rx_len = (vxt_word)sz;

	vxt_system_interrupt(VXT_GET_SYSTEM(n), 6);
	return VXT_NO_ERROR;
}

static vxt_error install(struct ebridge *n, vxt_system *s) {
	#ifdef _WIN32
		WSADATA ws_data;
		if (WSAStartup(MAKEWORD(2, 2), &ws_data)) {
			puts("ERROR: WSAStartup failed!");
			return -1;
		}
	#endif
	
	if ((n->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		VXT_LOG("Could not create socket!");
		return VXT_USER_ERROR(0);
	}
	
	VXT_PRINT("Ebridge server address: %s:%d", n->bridge_addr, n->rx_port);
	VXT_PRINT((n->rx_port != n->tx_port) ? ",%d\n" : "\n", n->tx_port);
	VXT_PRINT("Ebridge MTU: %d\n", MAX_PACKET_SIZE);

	n->addr.sin_family = AF_INET;
	n->addr.sin_addr.s_addr = inet_addr(n->bridge_addr);
	n->addr.sin_port = htons(n->tx_port);
	
	if (bind(n->sockfd, (struct sockaddr*)&n->addr, sizeof(n->addr)) < 0) {
		VXT_LOG("Bind failed");
		return VXT_USER_ERROR(1);
	}
	
	n->addr.sin_port = htons(n->rx_port);

	struct vxt_peripheral *p = VXT_GET_PERIPHERAL(n);
	vxt_system_install_io_at(s, p, 0xB2);
	vxt_system_install_timer(s, p, POLL_DELAY);
	return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(ebridge, {
	memcpy(DEVICE->mac_addr, default_mac, sizeof(default_mac));
	DEVICE->rx_port = DEVICE->tx_port = DEFAULT_PORT;
	
	const char *port_str = strchr(ARGS, ':');
	if (port_str) {
		sscanf(port_str + 1, "%hu,%hu", &DEVICE->rx_port, &DEVICE->tx_port);
		memcpy(DEVICE->bridge_addr, ARGS, port_str - ARGS);
	} else {
		strcpy(DEVICE->bridge_addr, ARGS);
	}

	PERIPHERAL->install = &install;
	PERIPHERAL->name = &name;
	PERIPHERAL->reset = &reset;
	PERIPHERAL->timer = &timer;
	PERIPHERAL->io.in = &in;
	PERIPHERAL->io.out = &out;
})
