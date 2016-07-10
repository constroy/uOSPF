#ifndef _LSDB_H
#define _LSDB_H

#include "packet.h"
#include "structure.h"

void gen_router_lsa(area *a);

void add_lsah(struct neighbor *nbr, const lsa_header *lsah);

lsa_header *find_lsa(const area *a, const lsa_header *lsah);

void insert_lsa(area *a, const lsa_header *lsah);

#endif
