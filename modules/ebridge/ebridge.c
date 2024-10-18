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

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif

#define MAX_PACKET_SIZE 0x5DC	// Hardcoded in the driver extension.
#define POLL_DELAY 10000		// 10ms
#define PORT 1235

struct ebridge {
	bool can_recv;
	
	int sockfd;
	struct sockaddr_in addr;
	char bridge_addr[64];

	// Temp buffer for package
	vxt_byte buffer[MAX_PACKET_SIZE];
	
	vxt_word pkg_len;
	vxt_word buf_seg;
	vxt_word buf_offset;
};

static vxt_byte in(struct ebridge *n, vxt_word port) {
	(void)n; (void)port;
	return 0; // Return 0 to indicate that we have a network card.
}

static void out(struct ebridge *n, vxt_word port, vxt_byte data) {
	/*
		This is the API of Fake86's packet driver
		with the VirtualXT extension.

		References:
			http://crynwr.com/packet_driver.html
			http://crynwr.com/drivers/
	*/

	(void)port; (void)data;
	vxt_system *s = VXT_GET_SYSTEM(n);
	struct vxt_registers *r = vxt_system_registers(s);

	switch (r->ah) {
		case 0: // Enable packet reception
			n->can_recv = true;
			break;
		case 1: // Send packet of CX at DS:SI
			if (r->cx > MAX_PACKET_SIZE) {
				VXT_LOG("Can't send! Invalid package size.");
				break;
			}

			for (int i = 0; i < (int)r->cx; i++)
				n->buffer[i] = vxt_system_read_byte(s, VXT_POINTER(r->ds, r->si + i));

			if (sendto(n->sockfd, (void*)n->buffer, r->cx, 0, (const struct sockaddr*)&n->addr, sizeof(n->addr)) != r->cx)
				VXT_LOG("Could not send packet!");
			break;
		case 2: // Return packet info (packet buffer in DS:SI, length in CX)
			r->ds = n->buf_seg;
			r->si = n->buf_offset;
			r->cx = n->pkg_len;
			break;
		case 3: // Copy packet to final destination (given in ES:DI)
			for (int i = 0; i < (int)r->cx; i++)
				vxt_system_write_byte(s, VXT_POINTER(r->es, r->di + i), vxt_system_read_byte(s, VXT_POINTER(n->buf_seg, n->buf_offset + i)));
			break;
		case 4: // Disable packet reception
			n->can_recv = false;
			break;
		case 5: // DEBUG
			break;
		case 0xFF: // Setup packet buffer
			n->buf_seg = r->cs;
			n->buf_offset = r->dx;
			VXT_LOG("Packet buffer is located at %04X:%04X", r->cs, r->dx);
			break;
	}
}

static vxt_error reset(struct ebridge *n, struct ebridge *state) {
	if (state)
		return VXT_CANT_RESTORE;
		
	n->pkg_len = n->buf_offset = 0;
	n->buf_seg = 0xD000;
	n->can_recv = false;
	return VXT_NO_ERROR;
}

static const char *name(struct ebridge *n) {
	(void)n; return "Network Adapter (Fake86 Interface)";
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
	
	ssize_t sz = recvfrom(n->sockfd, (void*)n->buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
	if (sz <= 0) {
		VXT_LOG("'recvfrom' failed!");
		return VXT_NO_ERROR;
	}

	n->can_recv = false;
	n->pkg_len = (vxt_word)sz;

	vxt_system *s = VXT_GET_SYSTEM(n);
	for (int i = 0; i < n->pkg_len; i++)
		vxt_system_write_byte(s, VXT_POINTER(n->buf_seg, n->buf_offset + i), n->buffer[i]);
		
	vxt_system_interrupt(s, 6);
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
	
	VXT_LOG("Server address: %s:%d", n->bridge_addr, PORT);
	VXT_LOG("Network MTU: %d", MAX_PACKET_SIZE);

	n->addr.sin_family = AF_INET;
	n->addr.sin_addr.s_addr = inet_addr(n->bridge_addr);
	n->addr.sin_port = htons(PORT + 1);
	
	if (bind(n->sockfd, (struct sockaddr*)&n->addr, sizeof(n->addr)) < 0) {
		VXT_LOG("Bind failed");
		return VXT_USER_ERROR(1);
	}
	
	n->addr.sin_port = htons(PORT);

	struct vxt_peripheral *p = VXT_GET_PERIPHERAL(n);
	vxt_system_install_io_at(s, p, 0xB2);
	vxt_system_install_timer(s, p, POLL_DELAY);
	return VXT_NO_ERROR;
}

VXTU_MODULE_CREATE(ebridge, {
	strncpy(DEVICE->bridge_addr, *ARGS ? ARGS : "127.0.0.1", sizeof(DEVICE->bridge_addr) - 1);

	PERIPHERAL->install = &install;
	PERIPHERAL->name = &name;
	PERIPHERAL->reset = &reset;
	PERIPHERAL->timer = &timer;
	PERIPHERAL->io.in = &in;
	PERIPHERAL->io.out = &out;
})
