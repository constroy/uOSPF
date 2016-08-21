#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "io.h"
#include "lsdb.h"
#include "recv.h"
#include "send.h"
#include "spf.h"

int main() {
	int area_id;
	int on = 1;
	pthread_t t_recv, t_send;
	all_spf_routers = inet_addr("224.0.0.5");
	all_d_routers = inet_addr("224.0.0.6");
	if_init();
	printf("%d interfaces is on.\n", num_if);
	for (int i = 0; i < num_if; ++i) {
		printf("%d: %s\n", i, ifs[i].name);
		puts("set area id for this interface:");
		scanf("%u", &area_id);
		areas[area_id].ifs[areas[area_id].num_if++] = ifs + i;
		ifs[i].a = areas + area_id;
		ifs[i].area_id = htonl(area_id);
		ifs[i].state = 0;
		ifs[i].hello_interval = 10;
		ifs[i].dead_interval = 40;
		ifs[i].rxmt_interval = 5;
		ifs[i].hello_timer = 0;
		ifs[i].wait_timer = 0;
		ifs[i].cost = htons(5);
		ifs[i].inf_trans_delay = 1;
		ifs[i].num_nbr = 0;
		ifs[i].nbrs = NULL;
	}
	/* use an ip address as my router id */
	myid = ifs[0].ip;

	pthread_create(&t_recv, NULL, recv_loop, &on);
	pthread_create(&t_send, NULL, send_loop, &on);
	/* update the route table */
	for (;;) {
		for (int i = 0; i < num_if; ++i) {
			rt_lsa = gen_router_lsa(ifs[i].a);
			if (ifs[i].state) {
				area *a = ifs[i].a;
				/* delete the old routes */
				for (int i = 0; i < a->num_route; ++i) {
					if (!a->routes[i].next) continue;
					static char cmd[128], ip[32], mask[32];
					strcpy(ip, inet_ntoa((struct in_addr){a->routes[i].ip}));
					strcpy(mask, inet_ntoa((struct in_addr){a->routes[i].mask}));
					sprintf(cmd,
							"route del -net %s netmask %s",	ip, mask);
					system(cmd);
					puts(cmd);
				}
				a->num_route = 0;
				update_table(ifs[i].a);
				/* add the new routes */
				for (int i = 0; i < a->num_route; ++i) {
					static char cmd[128], ip[32], mask[32], next[32];
					if (!a->routes[i].next) continue;
					strcpy(ip, inet_ntoa((struct in_addr){a->routes[i].ip}));
					strcpy(mask, inet_ntoa((struct in_addr){a->routes[i].mask}));
					strcpy(next, inet_ntoa((struct in_addr){a->routes[i].next}));
					sprintf(cmd,
							"route add -net %s netmask %s gw %s metric %hu dev %s",
							ip,
							mask,
							next,
							a->routes[i].metric,
							a->routes[i].iface);
					system(cmd);
					puts(cmd);
				}
				puts("--------------------------------");
				printf("Area %u Route Table:\n", ntohl(ifs[i].area_id));
				printf("IP\t\tMASK\t\tNext\t\tDistance\tInterface\n");
				for (int i = 0; i < a->num_route; ++i) {
					if (!a->routes[i].next) continue;
					/* Do not print them in one printf().
					   Because inet_ntoa() returns in a statically allocated buffer,
					   which subse‐quent calls will overwrite. */
					printf("%s\t", inet_ntoa((struct in_addr){a->routes[i].ip}));
					printf("%s\t", inet_ntoa((struct in_addr){a->routes[i].mask}));
					printf("%s\t", inet_ntoa((struct in_addr){a->routes[i].next}));
					printf("%hu\t", a->routes[i].metric);
					printf("%s\n", a->routes[i].iface);
				}
				puts("--------------------------------");

			}
		}
		sleep(5);
	}
	pthread_join(t_recv, NULL);
	pthread_join(t_send, NULL);
	return 0;
}
