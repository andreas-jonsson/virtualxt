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

#define _DEFAULT_SOURCE	1

#include "vxtp.h"

#include <string.h>
#include <pcap/pcap.h>

VXT_PIREPHERAL(network, {
    pcap_t *handle;
    bool can_recv;
    int pkg_len;
    vxt_byte buffer[0x10000];
})

static vxt_byte in(struct vxt_pirepheral *p, vxt_word port) {
    (void)p; (void)port;
	return 0; // Return 0 to indicate that we have a network card.
}

static void out(struct vxt_pirepheral *p, vxt_word port, vxt_byte data) {
	/*
		This is the API of Fake86's packet driver.

		References:
			http://crynwr.com/packet_driver.html
			http://crynwr.com/drivers/
	*/

    (void)port; (void)data;
    VXT_DEC_DEVICE(n, network, p);
    vxt_system *s = VXT_GET_SYSTEM(network, p);
    struct vxt_registers *r = vxt_system_registers(s);

	switch (r->ah) {
        case 0: // Enable packet reception
            n->can_recv = true;
            break;
        case 1: // Send packet of CX at DS:SI
            for (int i = 0; i < (int)r->cx; i++)
                n->buffer[i] = vxt_system_read_byte(s, VXT_POINTER(r->ds, r->si + i));
            
            if (pcap_sendpacket(n->handle, n->buffer, r->cx))
                fprintf(stderr, "Could not send packet!\n");
            break;
        case 2: // Return packet info (packet buffer in DS:SI, length in CX)
            r->ds = 0xD000;
            r->si = 0;
            r->cx = (vxt_word)n->pkg_len;
            break;
        case 3: // Copy packet to final destination (given in ES:DI)
            for (int i = 0; i < (int)r->cx; i++)
                vxt_system_write_byte(s, VXT_POINTER(r->es, r->di + i), vxt_system_read_byte(s, VXT_POINTER(0xD000, i)));
            break;
        case 4: // Disable packet reception
            n->can_recv = false;
            break;
	}
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    vxt_system_install_io_at(s, p, 0xB2);
    return VXT_NO_ERROR;
}

static vxt_error destroy(struct vxt_pirepheral *p) {
    vxt_system_allocator(VXT_GET_SYSTEM(network, p))(p, 0);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(n, network, p);
    n->can_recv = false;
    n->pkg_len = 0;
    return VXT_NO_ERROR;
}

static const char *name(struct vxt_pirepheral *p) {
    (void)p;
    return "Network Adapter (Fake86 Interface)";
}

static pcap_t *init_pcap(int device) {
    pcap_t *handle = NULL;
    pcap_if_t *devs = NULL;
    char buffer[PCAP_ERRBUF_SIZE] = {0};

    puts("Initialize network:");

	if (pcap_findalldevs(&devs, buffer)) {
        fprintf(stderr, "pcap_findalldevs() failed with error: %s\n", buffer);
        return NULL;
	}

    int i = 0;
    pcap_if_t *dev = NULL;
	for (dev = devs; dev; dev = dev->next) {
        if (++i == device)
            break;
    }

	if (!dev) {
        fprintf(stderr, "No network interface found! (Check device ID.)\n");
        pcap_freealldevs(devs);
        return NULL;
    }

    if (!(handle = pcap_open_live(dev->name, 0xFFFF, PCAP_OPENFLAG_PROMISCUOUS, -1, buffer))) {
        fprintf(stderr, "pcap error: %s\n", buffer);
        pcap_freealldevs(devs);
        return NULL;
    }

    printf("%d - %s\n", device, dev->name);
    pcap_freealldevs(devs);
    return handle;
}

struct vxt_pirepheral *vxtp_network_create(vxt_allocator *alloc, int device) {
    pcap_t *handle = NULL;
    if (!(handle = init_pcap(device)))
        return NULL;

    struct vxt_pirepheral *p = (struct vxt_pirepheral*)alloc(NULL, VXT_PIREPHERAL_SIZE(network));
    vxt_memclear(p, VXT_PIREPHERAL_SIZE(network));

    (VXT_GET_DEVICE(network, p))->handle = handle;

    p->install = &install;
    p->destroy = &destroy;
    p->name = &name;
    p->reset = &reset;
    p->io.in = &in;
    p->io.out = &out;
    return p;
}

vxt_error vxtp_network_list(int *prefered) {
    pcap_if_t *devs = NULL;
    char buffer[PCAP_ERRBUF_SIZE] = {0};

	if (pcap_findalldevs(&devs, buffer)) {
        fprintf(stderr, "pcap_findalldevs() failed with error: %s\n", buffer);
        return VXT_USER_ERROR(0);
	}

    puts("Available network devices:");

    int i = 0;
    pcap_if_t *selected = NULL;

	for (pcap_if_t *dev = devs; dev; dev = dev->next) {
        printf("%d - %s", ++i, dev->name);
        if (dev->description)
            printf(" (%s)", dev->description);
        
        if (!(dev->flags & PCAP_IF_LOOPBACK) && ((dev->flags & PCAP_IF_CONNECTION_STATUS) == PCAP_IF_CONNECTION_STATUS_CONNECTED)) {
            for (struct pcap_addr *pa = dev->addresses; pa; pa = pa->next) {
                if (pa->addr->sa_family == AF_INET) {
                    struct sockaddr_in *sin = (struct sockaddr_in*)pa->addr;
                    printf(" - %s", inet_ntoa(sin->sin_addr));
                    if (selected)
                        selected = dev;
                }
            }
        }
        puts("");
    }

    pcap_freealldevs(devs);
    if (prefered && selected)
        *prefered = i;
    return VXT_NO_ERROR;
}

vxt_error vxtp_network_poll(struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(n, network, p);
    vxt_system *s = VXT_GET_SYSTEM(network, p);

    if (!n->can_recv)
        return VXT_NO_ERROR;

    const vxt_byte *data = NULL;
    struct pcap_pkthdr *header = NULL;

    if (pcap_next_ex(n->handle, &header, &data) <= 0)
        return VXT_NO_ERROR;

    if (header->len > sizeof(n->buffer))
        return VXT_NO_ERROR;
    
    n->can_recv = false;
    n->pkg_len = header->len;

    for (int i = 0; i < n->pkg_len; i++)
        vxt_system_write_byte(s, VXT_POINTER(0xD000, i), data[i]);
    vxt_system_interrupt(s, 6);

    return VXT_NO_ERROR;
}
