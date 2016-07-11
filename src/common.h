#ifndef _COMMON_H
#define _COMMON_H

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "structure.h"

#define BUFFER_SIZE			2048

#define NUM_AREA			256
#define NUM_INTERFACE		256

extern const int max_age;
extern const int max_age_diff;

extern const char *ospf_type_name[];

extern int sock;
extern int num_area;
extern area areas[];
extern int num_if;
extern interface ifs[];
extern in_addr_t myid;
extern in_addr_t all_spf_routers;
extern in_addr_t all_d_routers;

int32_t get_lsa_seq();

#endif
