#include <net/ethernet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>

#include "common.h"
#include "dd.h"
#include "hello.h"
#include "io.h"
#include "lsdb.h"
#include "nsm.h"
#include "packet.h"

int sock;

void *send_loop(void *p) {
	uint8_t buf[BUFFER_SIZE];
	while (*(int *)p) {
		for (int i = 0; i < num_if; ++i) {
			/* delete neighbors out of date */
			/* BUG: mutex absent in critical area */
			struct neighbor **p = &ifs[i].nbrs;
			for (struct neighbor *q = *p; q; q = *p) {
				if (++q->inact_timer >= ifs[i].dead_interval) {
					--ifs[i].num_nbr;
					*p = q->next;
					free(q);
					gen_router_lsa(areas + ifs[i].area_id);
				} else {
					p = &q->next;
				}
			}
			/* send hello packet periodically */
			if (++ifs[i].hello_timer >= ifs[i].hello_interval) {
				ifs[i].hello_timer = 0;
				produce_hello(ifs + i,
							  (ospf_header *)(buf + sizeof(struct iphdr)));
				send_ospf(ifs + i, (struct iphdr *)buf, brd_addr);
			}
			/* send dd packet */
			for (struct neighbor *p = ifs[i].nbrs; p; p = p->next)
				if (p->state == S_ExStart || p->state == S_Exchange)
					if (++p->rxmt_timer >= ifs[i].rxmt_interval) {
						p->rxmt_timer = 0;
						produce_dd(ifs + i, p,
								   (ospf_header *)(buf + sizeof(struct iphdr)));
						send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
						if (!p->master && !p->more)
							add_event(ifs + i, p, E_ExchangeDone);
					}
		}
		sleep(1);
	}
	return NULL;
}

void *recv_loop(void *p) {
	uint8_t buf[BUFFER_SIZE];
	interface *iface;
	ospf_header *ospfhdr;
	struct neighbor *nbr;
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
			//case 3: proc_lsr(ospfhdr);break;
			//case 4: proc_lsu(ospfhdr);break;
			//case 5: proc_ack(ospfhdr);break;
			default: break; // TODO: Error
		}
	}
	return NULL;
}

int main() {
	int area_id;
	int on = 1;
	pthread_t t_send, t_recv;
	brd_addr = inet_addr("224.0.0.5");
	sock = socket(AF_PACKET, SOCK_DGRAM, htons(ETHERTYPE_IP));
	if_init(sock);
	printf("%d interfaces is on.\n", num_if);
	for (int i = 0; i < num_if; ++i) {
		printf("%d: %s\n", i, ifs[i].name);
		puts("please input area id for this interface:");
		scanf("%u", &area_id);
		areas[area_id].ifs[areas[area_id].num_if++] = ifs + i;
		ifs[i].area_id = htonl(area_id);
		ifs[i].state = 0;
		ifs[i].hello_interval = 10;
		ifs[i].dead_interval = 40;
		ifs[i].rxmt_interval = 5;
		ifs[i].hello_timer = 0;
		ifs[i].wait_timer = 0;
		ifs[i].cost = 5;
		ifs[i].inf_trans_delay = 1;
		ifs[i].num_nbr = 0;
		ifs[i].nbrs = NULL;
	}
	/* use an ip address as my router id */
	myid = ifs[0].ip;

	pthread_create(&t_send, NULL, send_loop, &on);
	pthread_create(&t_recv, NULL, recv_loop, &on);

	pthread_join(t_send, NULL);
	pthread_join(t_recv, NULL);
	return 0;
}
