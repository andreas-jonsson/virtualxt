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
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#define _DEFAULT_SOURCE	1

#include <vxt/vxtu.h>

#ifdef VXTU_STATIC_MODULES

    struct network { int _; };
    VXTU_MODULE_CREATE(network, { return NULL; })

#else

    #include <string.h>
    #include <stdlib.h>
    #include <pcap/pcap.h>

    #ifndef _WIN32
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
    #endif

    struct network {
        pcap_t *handle;
        bool can_recv;
        int pkg_len;
        char nif[128];
        vxt_byte buffer[0x10000];
    };

    static vxt_byte in(struct network *n, vxt_word port) {
        (void)n; (void)port;
        return 0; // Return 0 to indicate that we have a network card.
    }

    static void out(struct network *n, vxt_word port, vxt_byte data) {
        /*
            This is the API of Fake86's packet driver.

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
                for (int i = 0; i < (int)r->cx; i++)
                    n->buffer[i] = vxt_system_read_byte(s, VXT_POINTER(r->ds, r->si + i));
                
                if (pcap_sendpacket(n->handle, n->buffer, r->cx))
                    VXT_LOG("Could not send packet!");
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

    static vxt_error reset(struct network *n) {
        n->can_recv = false;
        n->pkg_len = 0;
        return VXT_NO_ERROR;
    }

    static const char *name(struct network *n) {
        (void)n; return "Network Adapter (Fake86 Interface)";
    }

    static vxt_error timer(struct network *n, vxt_timer_id id, int cycles) {
        (void)id; (void)cycles;
        vxt_system *s = VXT_GET_SYSTEM(n);

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

    static pcap_t *init_pcap(struct network *n) {
        pcap_t *handle = NULL;
        pcap_if_t *devs = NULL;
        char buffer[PCAP_ERRBUF_SIZE] = {0};

        puts("Initialize network:");

        if (pcap_findalldevs(&devs, buffer)) {
            VXT_LOG("pcap_findalldevs() failed with error: %s", buffer);
            return NULL;
        }

        int i = 0;
        pcap_if_t *dev = NULL;
        for (dev = devs; dev; dev = dev->next) {
            i++;
            if (!strcmp(dev->name, n->nif))
                break;
        }

        if (!dev) {
            VXT_LOG("No network interface found! (Check device name.)");
            pcap_freealldevs(devs);
            return NULL;
        }

        if (!(handle = pcap_open_live(dev->name, 0xFFFF, PCAP_OPENFLAG_PROMISCUOUS, -1, buffer))) {
            VXT_LOG("pcap error: %s", buffer);
            pcap_freealldevs(devs);
            return NULL;
        }

        printf("%d - %s\n", i, dev->name);
        pcap_freealldevs(devs);
        return handle;
    }

    static bool list_devices(int *prefered) {
        pcap_if_t *devs = NULL;
        char buffer[PCAP_ERRBUF_SIZE] = {0};

        if (pcap_findalldevs(&devs, buffer)) {
            VXT_LOG("pcap_findalldevs() failed with error: %s", buffer);
            return false;
        }

        puts("Host network devices:");

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
        return true;
    }

    static vxt_error install(struct network *n, vxt_system *s) {
        struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(n);
        if (!list_devices(NULL))
            return VXT_USER_ERROR(0);

        if (!(n->handle = init_pcap(n)))
            return VXT_USER_ERROR(1);

        vxt_system_install_io_at(s, p, 0xB2);
        vxt_system_install_timer(s, p, 10000);
        return VXT_NO_ERROR;
    }

    VXTU_MODULE_CREATE(network, {
        strncpy(DEVICE->nif, ARGS, sizeof(DEVICE->nif) - 1);

        PIREPHERAL->install = &install;
        PIREPHERAL->name = &name;
        PIREPHERAL->reset = &reset;
        PIREPHERAL->timer = &timer;
        PIREPHERAL->io.in = &in;
        PIREPHERAL->io.out = &out;
    })

#endif
