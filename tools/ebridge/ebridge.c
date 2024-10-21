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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pcap/pcap.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif

#if 0
	unsigned int debug_msg_count = 0;
	#define DEBUG(...) { printf("[%d] DEBUG: ", debug_msg_count++); printf(__VA_ARGS__); printf("\n"); }
#else
	#define DEBUG(...) {}
#endif

#define MAX_PACKET_SIZE 0xFFF0 // Should match MTU in the module.
#define PORT 1235

pcap_t *handle = NULL;
int sockfd = -1;
unsigned char buffer[MAX_PACKET_SIZE] = {0};
char nif[64] = {0};

static bool has_data(int fd) {
	if (fd == -1)
		return false;

	fd_set fds;
	FD_ZERO(&fds);
    FD_SET(fd, &fds);
	struct timeval timeout = {0};

    return select(fd + 1, &fds, NULL, NULL, &timeout) > 0;
}

static pcap_t *init_pcap(void) {
	pcap_t *handle = NULL;
	pcap_if_t *devs = NULL;
	char buffer[PCAP_ERRBUF_SIZE] = {0};

	puts("Initialize network:");

	if (pcap_findalldevs(&devs, buffer)) {
		printf("'pcap_findalldevs' failed with error: %s\n", buffer);
		return NULL;
	}

	int i = 0;
	pcap_if_t *dev = NULL;
	for (dev = devs; dev; dev = dev->next) {
		i++;
		if (!strcmp(dev->name, nif))
			break;
	}

	if (!dev) {
		puts("No network interface found! (Check device name.)");
		pcap_freealldevs(devs);
		return NULL;
	}

	handle = pcap_open(dev->name, 0xFFFF, PCAP_OPENFLAG_PROMISCUOUS|PCAP_OPENFLAG_NOCAPTURE_LOCAL, 1, NULL, buffer);
	if (!handle) {
		printf("pcap error: %s\n", buffer);
		pcap_freealldevs(devs);
		return NULL;
	}

	printf("%d - %s\n", i, dev->name);
	pcap_freealldevs(devs);
	
	if (*buffer)
		printf("pcap warning: %s\n", buffer);
	return handle;
}

static bool list_devices(int *prefered) {
	pcap_if_t *devs = NULL;
	char buffer[PCAP_ERRBUF_SIZE] = {0};

	if (pcap_findalldevs(&devs, buffer)) {
		printf("'pcap_findalldevs' failed with error: %s\n", buffer);
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

int main(int argc, char *argv[]) {
	#ifdef _WIN32
		WSADATA ws_data;
		if (WSAStartup(MAKEWORD(2, 2), &ws_data)) {
			puts("ERROR: WSAStartup failed!");
			return -1;
		}
	#endif
	
	if (argc < 2) {
		puts("Usage: ebridge <network-device>\n");
		list_devices(NULL);
		return -2;
	}
	strncpy(nif, argv[1], sizeof(nif) - 1);

	if (!(handle = init_pcap()))
		return -3;
		
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		puts("ERROR: Could not create socket!");
		return -4;
	}
	
	socklen_t addr_len = 0;
	struct sockaddr_in addr = {0};
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);
	
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		puts("ERROR: Bind failed");
		return -5;
	}
	
	addr.sin_port = htons(PORT + 1);
	
	puts("Waiting for packets...");
	
	for (;;) {
		if (!addr_len)
			sleep(1);

		ssize_t pkt_sz = 0;
		if (has_data(sockfd)) {
			socklen_t ln = sizeof(addr);
			pkt_sz = recvfrom(sockfd, (void*)buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &ln);
			if (pkt_sz <= 0) {
				DEBUG("'recvfrom' failed!");
				continue;
			}
			
			if (!addr_len) {
				addr_len = ln;
				puts("Connected to emulator!");
			} else {
				DEBUG("Package received!");
			}
		}

		if (!addr_len) {
			DEBUG("Waiting for initial connection...");
			continue;
		}
			
		if (pkt_sz > 0) {
			if (pcap_sendpacket(handle, buffer, pkt_sz))
				puts("WARNING: Could not send packet on physical network!");
		}
		
		const unsigned char *data = NULL;
		struct pcap_pkthdr *header = NULL;
		
		if ((pcap_next_ex(handle, &header, &data) <= 0) || (header->len == 0))
			continue;

		if (sendto(sockfd, (void*)data, header->len, 0, (const struct sockaddr*)&addr, sizeof(addr)) != (int)header->len)
			puts("WARNING: Could not send packet to emulator!");
		else
			DEBUG("Package sent: %d bytes", header->len);
	}

	close(sockfd);
	pcap_close(handle);
}
