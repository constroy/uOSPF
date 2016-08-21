#include "lsdb.h"

#include <string.h>

#include "common.h"

int32_t get_lsa_seq() {
	static int32_t lsa_seq = 0x80000001;
	return htonl(lsa_seq++);
}

/*
 * This function is from GNU Zebra.
 * Copyright (C) 1999, 2000 Toshiaki Takada
 * Fletcher Checksum -- Refer to RFC1008.
 */

#define MODX						4102
#define LSA_CHECKSUM_OFFSET			15

uint16_t fletcher16(const uint8_t *data, size_t len) {
    const uint8_t *sp, *ep, *p, *q;
	int c0 = 0, c1 = 0;
	int x, y;

	for (sp = data, ep = sp + len; sp < ep; sp = q) {
		q = sp + MODX;
		if (q > ep) q = ep;
		for (p = sp; p < q; p++) {
			c0 += *p;
			c1 += c0;
		}
		c0 %= 255;
		c1 %= 255;
	}
	x = ((len - LSA_CHECKSUM_OFFSET) * c0 - c1) % 255;
	if (x <= 0) x += 255;
	y = 510 - c0 - x;
	if (y > 255) y -= 255;
	return (x << 8) + y;
}

lsa_header *gen_router_lsa(area *a) {
	uint8_t buff[BUFFER_SIZE];
	lsa_header *lsah = (lsa_header *)buff;
	router_lsa *rlsa = (router_lsa *)(buff + sizeof(lsa_header));
	struct link *l = rlsa->links;
	lsah->age = 0;
	lsah->options = OPTIONS;
	lsah->type = OSPF_ROUTER_LSA;
	lsah->id = myid;
	lsah->ad_router = myid;
	lsah->seq_num = get_lsa_seq();
	rlsa->flags = 0x00;
	rlsa->zero = 0x00;

	for (int i = 0; i < a->num_if; ++i) {
		if (a->ifs[i]->num_nbr == 0) {
			/* StubNet */
			l->type = ROUTERLSA_STUB;
			l->id = a->ifs[i]->ip & a->ifs[i]->mask;
			l->data = a->ifs[i]->mask;
			l->tos = 0;
			l->metric = a->ifs[i]->cost;
			++l;
		//~ } else if (a->ifs[i]->num_nbr == 1) {
			//~ /* P2P */
			//~ if (a->ifs[i]->nbrs->state == S_Full) {
				//~ /* Router */
				//~ l->type = ROUTERLSA_ROUTER;
				//~ l->id = a->ifs[i]->nbrs->router_id;
				//~ l->data = a->ifs[i]->ip;
				//~ l->tos = 0;
				//~ l->metric = a->ifs[i]->cost;
				//~ ++l;
			//~ }
			//~ /* SubNet */
			//~ l->type = ROUTERLSA_STUB;
			//~ l->id = a->ifs[i]->ip & a->ifs[i]->mask;
			//~ l->data = a->ifs[i]->mask;
			//~ l->tos = 0;
			//~ l->metric = a->ifs[i]->cost;
			//~ ++l;
		} else {
			/* Broadcast */
			if (a->ifs[i]->state == 1) {
				/* TransNet */
				l->type = ROUTERLSA_TRANSIT;
				l->id = a->ifs[i]->dr;
				l->data = a->ifs[i]->ip;
				l->tos = 0;
				l->metric = a->ifs[i]->cost;
				++l;
			} else {
				/* StubNet */
				l->type = ROUTERLSA_STUB;
				l->id = a->ifs[i]->ip & a->ifs[i]->mask;
				l->data = a->ifs[i]->mask;
				l->tos = 0;
				l->metric = a->ifs[i]->cost;
				++l;
			}
		}
	}
	rlsa->num_link = htons((l - rlsa->links));
	size_t len = sizeof(lsa_header) + sizeof(router_lsa) +
				(l - rlsa->links) * sizeof(struct link);
	lsah->length = htons(len);
	lsah->checksum = 0;
	lsah->checksum = htons(fletcher16(buff + sizeof(lsah->age),
								ntohs(lsah->length) - sizeof(lsah->age)));
	return insert_lsa(a, lsah);
}

int lsah_eql(const lsa_header *a, const lsa_header *b) {
	return a->type == b->type && a->id == b->id && a->ad_router == b->ad_router;
}

/* return value < 0: b is newer, > 0: a is newer , = 0: the same */
int cmp_lsah(const lsa_header *a, const lsa_header *b) {
	return -1;
	if (a->seq_num != b->seq_num)
		return ntohl(a->seq_num) - ntohl(b->seq_num);
	else if (a->checksum != b->checksum)
		return ntohs(a->checksum) - ntohs(b->checksum);
	else if (ntohs(a->age) == max_age)
		return +1;
	else if (ntohs(b->age) == max_age)
		return -1;
	else if (abs(ntohs(a->age) - ntohs(b->age)) < max_age_diff)
		return ntohs(b->age) - ntohs(a->age);
	else
		return 0;
}

void add_lsah(neighbor *nbr, const lsa_header *lsah) {
	for (int i = 0; i < nbr->num_lsah; ++i)
		if (lsah_eql(&nbr->lsahs[i], lsah)) {
			if (cmp_lsah(&nbr->lsahs[i], lsah) < 0)
				nbr->lsahs[i] = *lsah;
			return;
		}
	nbr->lsahs[nbr->num_lsah++] = *lsah;
}

lsa_header *find_lsa(const area *a, const lsa_header *lsah) {
	for (int i = 0; i < a->num_lsa; ++i)
		if (lsah_eql(a->lsas[i], lsah)) return a->lsas[i];
	return NULL;
}

lsa_header *insert_lsa(area *a, const lsa_header *lsah) {
	int i;
	for (i = 0; i < a->num_lsa; ++i)
		if (lsah_eql(a->lsas[i], lsah)) {
			if (cmp_lsah(a->lsas[i], lsah) < 0) break;
			else return NULL;
		}
	size_t len = ntohs(lsah->length);
	a->lsas[i] = realloc(a->lsas[i], len);
	memcpy(a->lsas[i], lsah, len);
	a->num_lsa += i == a->num_lsa;
	return a->lsas[i];
}
