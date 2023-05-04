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

#include <vxt/vxtu.h>

#include <string.h>
#include <stdlib.h>
#include <pcap/pcap.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

void *pcap_lib_handle = NULL;

VXT_PIREPHERAL(network, {
    struct {
        int	(*sendpacket)(pcap_t*,const u_char*,int);
        int	(*findalldevs)(pcap_if_t**,char*);
        void (*freealldevs)(pcap_if_t*);
        pcap_t *(*open_live)(const char*,int,int,int,char*);
        int (*next_ex)(pcap_t*, struct pcap_pkthdr**, const u_char**);
    } pcap;

    pcap_t *handle;
    bool can_recv;
    int pkg_len;
    char nif[128];
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
            
            if (n->pcap.sendpacket(n->handle, n->buffer, r->cx))
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

static vxt_error timer(struct vxt_pirepheral *p, vxt_timer_id id, int cycles) {
    (void)id; (void)cycles;
    VXT_DEC_DEVICE(n, network, p);
    vxt_system *s = VXT_GET_SYSTEM(network, p);

    if (!n->can_recv)
        return VXT_NO_ERROR;

    const vxt_byte *data = NULL;
    struct pcap_pkthdr *header = NULL;

    if (n->pcap.next_ex(n->handle, &header, &data) <= 0)
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

	if (n->pcap.findalldevs(&devs, buffer)) {
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
        n->pcap.freealldevs(devs);
        return NULL;
    }

    if (!(handle = n->pcap.open_live(dev->name, 0xFFFF, PCAP_OPENFLAG_PROMISCUOUS, -1, buffer))) {
        VXT_LOG("pcap error: %s", buffer);
        n->pcap.freealldevs(devs);
        return NULL;
    }

    printf("%d - %s\n", i, dev->name);
    n->pcap.freealldevs(devs);
    return handle;
}

static bool list_devices(struct network *n, int *prefered) {
    pcap_if_t *devs = NULL;
    char buffer[PCAP_ERRBUF_SIZE] = {0};

	if (n->pcap.findalldevs(&devs, buffer)) {
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

    n->pcap.freealldevs(devs);
    if (prefered && selected)
        *prefered = i;
    return true;
}

static bool load_pcap(struct network *n) {
    #ifndef _WIN32
        #define LIB_NAME "libpcap.so.1"
        if (!pcap_lib_handle && !(pcap_lib_handle = dlopen(LIB_NAME, RTLD_LAZY))) {
            VXT_LOG(dlerror());
            return false;
        }
    #else
        #define dlsym(l, n) GetProcAddress((HMODULE)l, (LPCSTR)n)
        #define LIB_NAME "npcap.dll"
        if (!pcap_lib_handle && !(pcap_lib_handle = (void*)LoadLibrary(LIB_NAME))) {
            VXT_LOG("Could not load: " LIB_NAME);
            return false;
        }
    #endif

    const char *(*lib_version)(void) = dlsym(pcap_lib_handle, "pcap_lib_version");
    if (!lib_version) {
        VXT_LOG("ERROR: Could not get pcap version!");
        return false;
    }
    VXT_LOG("Pcap version: %s", lib_version());

    #define LOAD_SYM(name)                                                  \
        if (!(n->pcap. name = dlsym(pcap_lib_handle, "pcap_" # name))) {    \
            VXT_LOG("ERROR: Could not load: " # name);                      \
            return false;                                                   \
        }                                                                   \
    
    LOAD_SYM(sendpacket);
    LOAD_SYM(findalldevs);
    LOAD_SYM(freealldevs);
    LOAD_SYM(open_live);
    LOAD_SYM(next_ex);

    #undef LOAD_SYM

    list_devices(n, NULL);
    return true;
}

static vxt_error install(vxt_system *s, struct vxt_pirepheral *p) {
    VXT_DEC_DEVICE(n, network, p);
    if (!load_pcap(n))
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
