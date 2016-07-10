#ifndef _STRUCTURE_H
#define _STRUCTURE_H

#include <arpa/inet.h>

#include "packet.h"

typedef enum {
	S_Down,
	S_Init,
	S_2Way,
	S_ExStart,
	S_Exchange,
	S_Loading,
	S_Full
} nbr_state;

typedef enum {
	E_HelloReceived,
	E_1WayReceived,
	E_2WayReceived,
	E_AdjOK,
	E_NegotiationDone,
	E_ExchangeDone,
	E_LoadingDone
} nbr_event;

typedef struct {
	nbr_state src_state;
	nbr_event event;
	nbr_state dst_state;
} transition;

struct neighbor {
	int inact_timer;					/* Inactivity Timer */
	int rxmt_timer;
	nbr_state state;
	int master;
	int more;
	uint32_t dd_seq_num;
	//unsigned int lsr_sequence;      /* 最终 LSR 使用的序号 */
	in_addr_t router_id;         /* Router ID */
	in_addr_t ip;                /* IP */
	uint8_t priority;
	int num_lsah;
	lsa_header lsahs[64];
	//int lsa_headers_state[10];
	//int lsa_headers_count;
	struct neighbor *next;
};

typedef struct {
	char name[16];
	int state;
	int sock;
	in_addr_t area_id;
	in_addr_t ip;
	in_addr_t mask;
	in_addr_t dr;
	in_addr_t bdr;
	int hello_interval;
	int dead_interval;
	int hello_timer;
	int wait_timer;
	uint16_t cost;
	uint16_t inf_trans_delay;
	int rxmt_interval;
	int num_nbr;
	struct neighbor *nbrs;
} interface;

typedef struct {
	int num_if;
	interface *ifs[16];
	uint32_t num_lsa;
	lsa_header *lsas[256];
	//~ int my_lsa_sequence;
	//~ struct as_external_lsa ases[10];        /* 单独存一个非本 Area 的外部第五类 */
	//~ int ases_count;
	//~ struct ip_list ip_lists[10];            /* FIXME: IP 列表 （下标就是 lsdb_items 的下标） */
	//~ int ip_lists_count;
	//~ struct path paths[10];                  /* 路由路径 */
	//~ int paths_count;
} area;

#endif
