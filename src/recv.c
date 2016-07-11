#include "recv.h"

#include "lsack.h"
#include "common.h"
#include "dd.h"
#include "hello.h"
#include "lsr.h"
#include "lsu.h"
#include "io.h"

void *recv_loop(void *p) {
	uint8_t buf[BUFFER_SIZE];
	interface *iface;
	ospf_header *ospfhdr;
	neighbor *nbr;
	in_addr_t src;
	while (*(int *)p) {
		iface = recv_ospf(sock, buf, BUFFER_SIZE, &src);
		ospfhdr = (ospf_header *)(buf + sizeof(struct iphdr));
		/* check if the packet is from myself */
		if (ospfhdr->router_id == myid) continue;
		for (nbr = iface->nbrs; nbr; nbr = nbr->next)
			if (ospfhdr->router_id == nbr->router_id) break;
		switch (ospfhdr->type) {
			case OSPF_TYPE_HELLO: process_hello(iface, nbr, ospfhdr, src); break;
			case OSPF_TYPE_DD: process_dd(iface, nbr, ospfhdr); break;
			case OSPF_TYPE_LSR: process_lsr(nbr, ospfhdr);break;
			case OSPF_TYPE_LSU: process_lsu(iface->a, nbr, ospfhdr);break;
			case OSPF_TYPE_LSACK: process_ack(nbr, ospfhdr);break;
			default: break;
		}
	}
	return NULL;
}
