#include <net/ethernet.h>
#include <pthread.h>
#include <stdio.h>

#include "common.h"
#include "io.h"
#include "recv.h"
#include "send.h"

int main() {
	int area_id;
	int on = 1;
	pthread_t t_recv, t_send;
	all_spf_routers = inet_addr("224.0.0.5");
	all_d_routers = inet_addr("224.0.0.6");
	sock = socket(AF_PACKET, SOCK_DGRAM, htons(ETHERTYPE_IP));
	if_init(sock);
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

	pthread_join(t_recv, NULL);
	pthread_join(t_send, NULL);
	return 0;
}
