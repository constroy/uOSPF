#ifndef _LSDB_H
#define _LSDB_H

#include "packet.h"
#include "structure.h"

uint16_t fletcher16(const uint8_t *data, size_t len);

void gen_router_lsa(area *a);

void add_lsah(neighbor *nbr, const lsa_header *lsah);

int lsah_eql(const lsa_header *a, const lsa_header *b);

/* return value < 0: b is newer, > 0: a is newer , = 0: the same */
int cmp_lsah(const lsa_header *a, const lsa_header *b);

lsa_header *find_lsa(const area *a, const lsa_header *lsah);

void insert_lsa(area *a, const lsa_header *lsah);

#endif
