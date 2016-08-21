#include "send.h"

#include "lsack.h"
#include "common.h"
#include "dd.h"
#include "hello.h"
#include "io.h"
#include "lsdb.h"
#include "lsr.h"
#include "lsu.h"
#include "nsm.h"

void *send_loop(void *p) {
	uint8_t buf[BUFFER_SIZE];
	while (*(int *)p) {
		/* BUG: mutex absent in critical area */
		for (int i = 0; i < num_if; ++i) {
			/* delete neighbors out of date */
			neighbor **p = &ifs[i].nbrs;
			for (neighbor *q = *p; q; q = *p) {
				if (++q->inact_timer >= ifs[i].dead_interval) {
					--ifs[i].num_nbr;
					*p = q->next;
					free(q);
				} else {
					p = &q->next;
				}
			}
			/* send hello packet periodically */
			if (++ifs[i].hello_timer >= ifs[i].hello_interval) {
				ifs[i].hello_timer = 0;
				produce_hello(ifs + i,
							  (ospf_header *)(buf + sizeof(struct iphdr)));
				send_ospf(ifs + i, (struct iphdr *)buf, all_spf_routers);
			}

			for (neighbor *p = ifs[i].nbrs; p; p = p->next) {
				if (++p->rxmt_timer >= ifs[i].rxmt_interval) {
					p->rxmt_timer = 0;
					/* send dd packet */
					if (p->state == S_ExStart || p->state == S_Exchange) {
						produce_dd(ifs + i, p,
								   (ospf_header *)(buf + sizeof(struct iphdr)));
						send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
						if (!p->master && !p->more)
							add_event(ifs + i, p, E_ExchangeDone);
					}
					/* send lsr packet */
					if (p->state == S_Exchange || p->state == S_Loading) {
						if (p->num_lsah) {
							produce_lsr(p, (ospf_header *)(buf + sizeof(struct iphdr)));
							send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
						} else {
							add_event(ifs + i, p, E_LoadingDone);
						}
					}
					/* send lsu packet */
					if (p->num_lsr && (p->state == S_Exchange ||
						p->state == S_Loading ||
						p->state == S_Full)) {
						produce_lsu(ifs[i].a, p,
								(ospf_header *)(buf + sizeof(struct iphdr)));
						send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
					}
					if (rt_lsa && p->state == S_Full) {
						produce_upd(ifs[i].a, rt_lsa,
								(ospf_header *)(buf + sizeof(struct iphdr)));
						send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
						rt_lsa = NULL;
					}
				}
				/* send lsack packet */
				if (p->state >= S_Exchange && p->num_ack) {
					produce_ack(p, (ospf_header *)(buf + sizeof(struct iphdr)));
					send_ospf(ifs + i, (struct iphdr *)buf, p->ip);
					p->num_ack = 0;
				}
			}
		}
		sleep(1);
	}
	return NULL;
}
