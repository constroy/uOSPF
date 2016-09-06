#include "spf.h"

#include <stdio.h>

#include "common.h"
#include "packet.h"

#define NUM_NODE		256
#define DISTANCE_INF	0x7fff

typedef struct {
	in_addr_t id;
	in_addr_t mask;
	in_addr_t next;
	uint16_t dist;
	const lsa_header *lsa;
} node;

int num_node;
node nodes[NUM_NODE];

int find_node(in_addr_t id) {
	int i;
	for (i = 0; i < num_node; ++i)
		if (nodes[i].id == id) break;
	return i;
}

void dijkstra(int root) {
	node *leaf = nodes + num_node;
	int use[NUM_NODE] = {};
	int pre[NUM_NODE];
	nodes[root].dist = 0;
	pre[root] = root;
	for (;;) {
		int p = -1, q;
		/* find out the nearest unused node */
		for (int j = 0; j < num_node; ++j)
			if (!use[j] && (p == -1 || nodes[p].dist > nodes[j].dist))
				p = j;
		if (p == -1 || nodes[p].dist == DISTANCE_INF) break;
		use[p] = 1;
		/* update distance of the rest nodes */
		if (nodes[p].lsa->type == OSPF_ROUTER_LSA) {
			router_lsa *rlsa = (router_lsa *)((uint8_t *)nodes[p].lsa +
					sizeof(lsa_header));
			int j = ntohs(rlsa->num_link);
			nodes[p].mask = 0;
			for (const struct link *l = rlsa->links; j--; ++l) {
				int k;
				switch (l->type) {
					case ROUTERLSA_ROUTER:
					case ROUTERLSA_TRANSIT:
						k = find_node(l->id);
						break;
					case ROUTERLSA_STUB:
						printf("leaf: %s\n", inet_ntoa((struct in_addr){l->id}));
						leaf->id = l->id;
						leaf->mask = l->data;
						leaf->next = nodes[p].next;
						leaf->dist = nodes[p].dist + ntohs(l->metric);
						leaf->lsa = nodes[p].lsa;
						pre[leaf - nodes] = p;
						++leaf;
					default:
						continue;
				}
				if (k == num_node || use[k]) continue;
				int det = ntohs(l->metric);
				if (nodes[k].dist > nodes[p].dist + det) {
					nodes[k].dist = nodes[p].dist + det;
					pre[k] = p;
				}
			}
		} else if (nodes[p].lsa->type == OSPF_NETWORK_LSA) {
			network_lsa *nlsa = (network_lsa *)((uint8_t *)nodes[p].lsa +
					sizeof(lsa_header));
			int j = (ntohs(nodes[p].lsa->length) - sizeof(lsa_header) -
					sizeof(nlsa->mask)) /sizeof(in_addr_t);
			nodes[p].mask = nlsa->mask;
			for (const in_addr_t *rt = nlsa->routers; j--; ++rt) {
				int k = find_node(*rt);
				if (k == num_node || use[k]) continue;
				router_lsa *rlsa = (router_lsa *)((uint8_t *)nodes[k].lsa +
						sizeof(lsa_header));
				int num = ntohs(rlsa->num_link);
				while (num--) {
					if (rlsa->links[num].type == ROUTERLSA_TRANSIT &&
						rlsa->links[num].id == nodes[p].id) break;
					if (rlsa->links[num].type == ROUTERLSA_STUB &&
						rlsa->links[num].id == (nodes[p].id & nlsa->mask)) break;
				}
				if (num < 0) continue;
				int det = ntohs(rlsa->links[num].metric);
				if (nodes[k].dist > nodes[p].dist + det) {
					nodes[k].dist = nodes[p].dist + det;
					pre[k] = p;
				}
			}
		}
		/* calculate the next router id */
		for (q = p; q != root; q = pre[q])
			if (!nodes[q].mask) nodes[p].next = nodes[q].id;
	}
	num_node = leaf - nodes;
}

void spt(lsa_header *const lsas[], int num_lsa) {
	int root;
	num_node = 0;
	/* lsa type 1 and lsa type 2 */
	for (int i = 0 ; i < num_lsa; ++i) {
		if (lsas[i]->type == OSPF_ROUTER_LSA || lsas[i]->type == OSPF_NETWORK_LSA) {
			nodes[num_node].id = lsas[i]->id;
			nodes[num_node].lsa = lsas[i];
			nodes[num_node].dist = DISTANCE_INF;
			if (nodes[num_node].id == myid) root = num_node;
			++num_node;
		}
	}
	dijkstra(root);
	/* lsa type 3 ans lsa type 4 */
	for (int i = 0 ; i < num_lsa; ++i) {
		if (lsas[i]->type == OSPF_SUMMARY_LSA ||
			lsas[i]->type == OSPF_ASBR_SUMMARY_LSA) {
			summary_lsa *slsa = (summary_lsa *)((uint8_t *)lsas[i] +
					sizeof(lsa_header));
			if (find_node(lsas[i]->id) < num_node) continue;
			int k = find_node(lsas[i]->ad_router);
			if (k == num_node || nodes[k].dist == DISTANCE_INF) continue;
			nodes[num_node].id = lsas[i]->id;
			nodes[num_node].mask = slsa->mask;
			nodes[num_node].next = nodes[k].next;
			nodes[num_node].dist = nodes[k].dist + ntohl(slsa->metric >> 4 << 4);
			nodes[num_node].lsa = lsas[i];
			++num_node;
		}
	}
	/* lsa type 5 */
	//TODO
}

in_addr_t find_nbr_ip(const area *a, in_addr_t id) {
	for (int i = 0; i < a->num_if; ++i)
		for (neighbor *p = a->ifs[i]->nbrs; p; p = p->next)
			if (p->router_id == id) return p->ip;
	return 0;
}

const char *find_if_name(const area *a, in_addr_t ip) {
	for (int i = 0; i < a->num_if; ++i)
		if ((a->ifs[i]->ip & a->ifs[i]->mask) == (ip & a->ifs[i]->mask))
			return a->ifs[i]->name;
	return 0;
}

void update_table(area *a) {
	spt(a->lsas, a->num_lsa);
	//
	for (int i = 0; i < num_node; ++i) {
		printf("node: %s\n", inet_ntoa((struct in_addr){nodes[i].id}));
		printf("next: %s\n", inet_ntoa((struct in_addr){nodes[i].next}));
		printf("dist: %hu\n", nodes[i].dist);
	}
	//
	for (int i = 0; i < num_node; ++i)
		if (nodes[i].mask && nodes[i].dist < DISTANCE_INF) {
			a->routes[a->num_route].mask = nodes[i].mask;
			a->routes[a->num_route].ip = nodes[i].id & a->routes[a->num_route].mask;
			a->routes[a->num_route].next = find_nbr_ip(a, nodes[i].next);
			if (a->routes[a->num_route].next) {
				a->routes[a->num_route].iface =
						find_if_name(a, a->routes[a->num_route].next);
			} else {
				a->routes[a->num_route].iface =
					find_if_name(a, a->routes[a->num_route].ip);
			}
			a->routes[a->num_route].metric = nodes[i].dist;
			++a->num_route;
		}
}
