#include "hello.h"

#include <time.h>

#include "common.h"
#include "lsdb.h"
#include "nsm.h"

void process_hello(interface *iface,
				   neighbor *nbr,
				   const ospf_header *ospfhdr,
				   in_addr_t src) {
	ospf_hello *hello = (ospf_hello *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	int num = (ntohs(ospfhdr->length) - (sizeof(ospf_header) + 20)) / 4;
	const in_addr_t *const nbrs = (in_addr_t *)hello->neighbors;
	/* create a new neighbor node in nbrs list */
	if (!nbr) {
		nbr = malloc(sizeof(neighbor));
		nbr->state = S_Down;
		nbr->rxmt_timer = 0;
		nbr->master = 1;
		nbr->more = 1;
		nbr->dd_seq_num = clock();
		nbr->router_id = ospfhdr->router_id;
		nbr->ip = src;
		nbr->priority = hello->priority;
		nbr->num_lsah = 0;
		nbr->num_lsr = 0;
		nbr->next = iface->nbrs;
		iface->nbrs = nbr;
		++iface->num_nbr;
		gen_router_lsa(iface->a);
	}
	nbr->inact_timer = 0;
	if (iface->dr != hello->d_router) {
		iface->dr = hello->d_router;
		gen_router_lsa(iface->a);
	}
	iface->bdr = hello->bd_router;
	add_event(iface, nbr, E_HelloReceived);
	while (num--) {
		if (nbrs[num] == myid) {
			add_event(iface, nbr, E_2WayReceived);
			break;
		}
	}
	if (num == -1) add_event(iface, nbr, E_1WayReceived);
	if (hello->d_router == src || hello->d_router == iface->ip ||
		hello->bd_router == src || hello->bd_router == iface->ip)
			add_event(iface, nbr, E_AdjOK);

}

void produce_hello(const interface *iface, ospf_header *ospfhdr) {
	ospf_hello *hello = (ospf_hello *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	in_addr_t *nbr = hello->neighbors;

	hello->network_mask = iface->mask;
	hello->hello_interval = htons(iface->hello_interval);
	/* support NSSA */
	hello->options = 0x08;
	/* avoid becoming DR or BDR */
	hello->priority = 0;
	hello->dead_interval = htonl(iface->dead_interval);
	hello->d_router = iface->dr;
	hello->bd_router = iface->bdr;
	for (const neighbor *p = iface->nbrs; p; p = p->next)
		*nbr++ = p->router_id;
	ospfhdr->type = OSPF_TYPE_HELLO;
	ospfhdr->length = htons((uint8_t *)nbr - (uint8_t *)ospfhdr);
}
