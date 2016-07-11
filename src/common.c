#include "common.h"

const int max_age = 3600;
const int max_age_diff = 900;

const char *ospf_type_name[] = {"Unknown", "Hello", "DD", "LSR", "LSU", "LSAck"};

int sock;
int num_area;
area areas[NUM_AREA];
int num_if;
interface ifs[NUM_INTERFACE];
in_addr_t myid;
in_addr_t all_spf_routers;
in_addr_t all_d_routers;

int32_t get_lsa_seq() {
	static int32_t lsa_seq = 0x80000001;
	return htonl(lsa_seq++);
}
