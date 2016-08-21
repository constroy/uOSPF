#include "lsu.h"

#include <string.h>

#include "lsdb.h"

void process_lsu(area *a, neighbor *nbr, ospf_header *ospfhdr) {
	uint8_t *p = (uint8_t *)ospfhdr + sizeof(ospf_header) + 4;
	int num = ntohl(*((uint32_t *)((uint8_t *)ospfhdr + sizeof(ospf_header))));
	while (num--) {
		lsa_header *lsah = (lsa_header *)p;
		uint16_t sum = ntohs(lsah->checksum);
		lsah->checksum = 0;
		/* check the checksum */
		if (sum != fletcher16(p + sizeof(lsah->age),
				ntohs(lsah->length) - sizeof(lsah->age))) continue;
		lsah->checksum = htons(sum);
		for (int i = 0; i < nbr->num_lsah; ++i)
			if (lsah_eql(nbr->lsahs +i, lsah)) {
				nbr->lsahs[i] = nbr->lsahs[--nbr->num_lsah];
				break;
			}
		/* add it to the ack list */
		nbr->acks[nbr->num_ack++] = *lsah;
		/* insert it to the lsdb */
		insert_lsa(a, lsah);
		p += ntohs(lsah->length);
	}
}

void produce_lsu(const area *a, const neighbor *nbr, ospf_header *ospfhdr) {
	uint8_t *p = (uint8_t *)ospfhdr + sizeof(ospf_header) + 4;
	*((uint32_t *)((uint8_t *)ospfhdr + sizeof(ospf_header))) =
			ntohl(nbr->num_lsr);
	for (int i = 0; i < nbr->num_lsr; ++i)
		for (int j = 0; j < a->num_lsa; ++j)
			if (nbr->lsrs[i].ls_type == htonl(a->lsas[j]->type) &&
				nbr->lsrs[i].ls_id == a->lsas[j]->id &&
				nbr->lsrs[i].ad_router == a->lsas[j]->ad_router) {
				size_t len = htons(a->lsas[j]->length);
				memcpy(p, a->lsas[j], len);
				p += len;
				break;
			}
	ospfhdr->type = OSPF_TYPE_LSU;
	ospfhdr->length = htons(p - (uint8_t *)ospfhdr);
}

void produce_upd(const area *a, const lsa_header *lsa, ospf_header *ospfhdr) {
	uint8_t *p = (uint8_t *)ospfhdr + sizeof(ospf_header) + 4;
	*((uint32_t *)((uint8_t *)ospfhdr + sizeof(ospf_header))) = ntohl(1);
	size_t len = htons(lsa->length);
	memcpy(p, lsa, len);
	p += len;
	ospfhdr->type = OSPF_TYPE_LSU;
	ospfhdr->length = htons(p - (uint8_t *)ospfhdr);
}
