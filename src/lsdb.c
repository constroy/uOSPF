#include "lsdb.h"

#include <string.h>

#include "common.h"

uint16_t fletcher16(const uint8_t *data, size_t len) {
	uint16_t sum1 = 0xff, sum2 = 0xff;
	size_t tlen;

	while (len) {
		tlen = len >= 20 ? 20 : len;
		len -= tlen;
		do {
				sum2 += sum1 += *data++;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}
	/* Second reduction step to reduce sums to 8 bits */
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	return sum2 << 8 | sum1;
}

void gen_router_lsa(area *a) {
	uint8_t buff[BUFFER_SIZE];
	lsa_header *lsah = (lsa_header *)buff;
	router_lsa *rlsa = (router_lsa *)(buff + sizeof(lsa_header));
	struct link *l = rlsa->links;
	lsah->age = 0;
	lsah->options = 0x08;
	lsah->type = OSPF_ROUTER_LSA;
	lsah->id = myid;
	lsah->ad_router = myid;
	lsah->seq_num = get_lsa_seq();
	rlsa->flags = 0x00;
	rlsa->zero = 0x00;

	int num = 0;
	for (int i = 0; i < a->num_if; ++i) {
		if (a->ifs[i]->num_nbr == 0) {
			/* StubNet */
			l->type = ROUTERLSA_STUB;
			l->id = a->ifs[i]->ip & a->ifs[i]->mask;
			l->data = a->ifs[i]->mask;
			l->tos = 0;
			l->metric = a->ifs[i]->cost;
			++l;
		} else if (a->ifs[i]->num_nbr == 1) {
			/* P2P */
			if (a->ifs[i]->nbrs->state == S_Full) {
				/* Router */
				l->type = ROUTERLSA_ROUTER;
				l->id = a->ifs[i]->nbrs->router_id;
				l->data = a->ifs[i]->ip;
				l->tos = 0;
				l->metric = a->ifs[i]->cost;
				++l;
			}
			/* SubNet */
			l->type = ROUTERLSA_STUB;
			l->id = a->ifs[i]->ip & a->ifs[i]->mask;
			l->data = a->ifs[i]->mask;
			l->tos = 0;
			l->metric = a->ifs[i]->cost;
			++l;
		} else {
			/* Broadcast */
			if (a->ifs[i]->state == 1) {
				/* TransNet */
				l->type = ROUTERLSA_TRANSNET;
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
	rlsa->num_link = htons(num);
	size_t len = sizeof(lsa_header) + sizeof(router_lsa) +
				 num * sizeof(struct link);
	/* length excluding the size of age */
	lsah->length = htons(len - sizeof(lsah->age));
	lsah->checksum = 0;
	lsah->checksum = fletcher16(buff + sizeof(lsah->age),
								ntohs(lsah->length));
	insert_lsa(a, lsah);
}

int lsah_eql(const lsa_header *a, const lsa_header *b) {
	return a->type == b->type && a->id == b->id && a->ad_router == b->ad_router;
}

/* return value < 0: b is newer, > 0: a is newer , = 0: the same */
int lsa_cmp(const lsa_header *a, const lsa_header *b) {
	if (a->seq_num != b->seq_num) return a->seq_num - b->seq_num;
	else if (a->checksum != b->checksum) return a->checksum - b->checksum;
	else if (a->age == max_age) return +1;
	else if (b->age == max_age) return -1;
	else if (abs(a->age - b->age) < max_age_diff) return b->age - a->age;
	else return 0;
}

void add_lsah(struct neighbor *nbr, const lsa_header *lsah) {
	for (int i = 0; i < nbr->num_lsah; ++i)
		if (lsah_eql(&nbr->lsahs[i], lsah)) return;
	nbr->lsahs[nbr->num_lsah++] = *lsah;
}

lsa_header *find_lsa(const area *a, const lsa_header *lsah) {
	for (int i = 0; i < a->num_lsa; ++i)
		if (lsah_eql(a->lsas[i], lsah)) return a->lsas[i];
	return NULL;
}

void insert_lsa(area *a, const lsa_header *lsa) {
	size_t len = ntohs(lsa->length) + sizeof(lsa->age);
	int i;
	for (i = 0; i < a->num_lsa; ++i)
		if (lsah_eql(a->lsas[i], lsa) && lsa_cmp(a->lsas[i], lsa) < 0)
			break;
	a->lsas[i] = realloc(a->lsas[i], len);
	memcpy(a->lsas[i], lsa, len);
	a->num_lsa += a->num_lsa == i;
}
