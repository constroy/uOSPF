#include "io.h"

#include <errno.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "common.h"

#define IPPROTO_OSPF 89

void if_init(int sock) {
	struct ifconf ifc;
	struct ifreq ifrs[NUM_INTERFACE];
	ifc.ifc_len = sizeof(ifrs);
	ifc.ifc_req = ifrs;
	ioctl(sock, SIOCGIFCONF, &ifc);
	int num = ifc.ifc_len / sizeof(struct ifreq);
	num_if = 0;
	for (int i = 0; i < num; ++i) {
		/* except loop-back */
		if (strcmp(ifrs[i].ifr_name, "lo")) {
			/* interface name */
			strcpy(ifs[num_if].name, ifrs[i].ifr_name);
			/* ip address */
			ioctl(sock, SIOCGIFADDR, ifrs + i);
			ifs[num_if].ip =
			((struct sockaddr_in *)&ifrs[i].ifr_addr)->sin_addr.s_addr;
			/* subnet mask */
			ioctl(sock, SIOCGIFNETMASK, ifrs + i);
			ifs[num_if].mask =
			((struct sockaddr_in *)&ifrs[i].ifr_netmask)->sin_addr.s_addr;
			/* turn on promisc mode */
			ioctl(sock, SIOCGIFFLAGS, ifrs + i);
			ifrs[i].ifr_flags |= IFF_PROMISC;
			ioctl(sock, SIOCSIFFLAGS, ifrs + i);
			/* bind socket to this interface */
			ifs[num_if].sock = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF);
			setsockopt(ifs[num_if].sock, SOL_SOCKET, SO_BINDTODEVICE,
					ifrs + i, sizeof(struct ifreq));
			++num_if;
		}
	}
}

uint16_t cksum(const uint16_t *data, size_t len) {
	uint32_t sum = 0;
	while (len > 1) {
		sum += *data++;
		len -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (len) sum += *(uint8_t *)data;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

interface *recv_ospf(int sock, uint8_t buf[], int size, in_addr_t *src) {
	struct iphdr *iph;
	ospf_header *ospfhdr;
	for (;;) {
		if (recvfrom(sock, buf, size, 0, NULL, NULL) < sizeof(struct iphdr))
			continue;
		iph = (struct iphdr *)buf;
		*src = iph->saddr;
		if (iph->protocol == IPPROTO_OSPF) {
			ospfhdr = (ospf_header *)(buf + sizeof(struct iphdr));
			memset(ospfhdr->auth_data, 0, sizeof(ospfhdr->auth_data));
			if (cksum((uint16_t *)ospfhdr, ntohs(ospfhdr->length))) continue;
			printf("%s received from %s\n", ospf_type_name[ospfhdr->type],
				inet_ntoa((struct in_addr){*src}));
			for (int i = 0; i < num_if; ++i)
				if ((iph->saddr & ifs[i].mask) == (ifs[i].ip & ifs[i].mask)) {
					return ifs + i;
			}
		}
	}
}

void send_ospf(const interface *iface, struct iphdr *iph, in_addr_t dst) {
	static int id;
	struct sockaddr_in addr;
	ospf_header * ospfhdr = (ospf_header *)((uint8_t *)iph + sizeof(struct iphdr));
	/* fill out the ospf header */
	ospfhdr->version = 2;
	ospfhdr->router_id = myid;
	ospfhdr->area_id = iface->area_id;
	ospfhdr->checksum = 0x0;
	ospfhdr->auth_type = 0;
	memset(ospfhdr->auth_data, 0, sizeof ospfhdr->auth_data);
	ospfhdr->checksum = cksum((uint16_t *)ospfhdr, ntohs(ospfhdr->length));
	/* fill out the ip header */
	iph->ihl = 5;
	iph->version= 4;
	iph->tos = 0xc0;
	iph->tot_len = htons(sizeof(struct iphdr) + ospfhdr->length);
	iph->id = htons(id++);
	iph->frag_off = 0x0;
	iph->ttl = 1;
	iph->protocol = IPPROTO_OSPF;
	iph->saddr = iface->ip;
	iph->daddr = dst;
	iph->check = 0x0;
	iph->check = cksum((uint16_t*)iph, sizeof(struct iphdr));
	/* send the packet */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = dst;
	sendto(iface->sock, ospfhdr, ntohs(ospfhdr->length), 0, (struct sockaddr *)&addr, sizeof(addr));
	printf("%s sent to %s\n", ospf_type_name[ospfhdr->type],
				inet_ntoa((struct in_addr){dst}));
}
