#include "lsack.h"

void process_ack(neighbor *nbr, ospf_header *ospfhdr) {
	if (nbr->state < S_Exchange) return;
	lsa_header *lsah = (lsa_header *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	int num = (ntohs(ospfhdr->length) - sizeof(ospf_header)) / sizeof(lsa_header);
	while (num--) {
		for (int i = 0; i < nbr->num_lsr; ++i)
			if (nbr->lsrs[i].ls_type == lsah[num].type &&
				nbr->lsrs[i].ls_id == lsah[num].id &&
				nbr->lsrs[i].ad_router == lsah[num].ad_router) {
					nbr->lsrs[i] = nbr->lsrs[--nbr->num_lsr];
					break;
				}
	}
}

void produce_ack(const neighbor *nbr, ospf_header *ospfhdr) {
	lsa_header *lsah = (lsa_header *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	for (int i = 0; i < nbr->num_ack; ++i) {
		*lsah++ = nbr->acks[i];
	}
	ospfhdr->type = OSPF_TYPE_LSACK;
	ospfhdr->length = htons((uint8_t*)lsah - (uint8_t *)ospfhdr);
}
