#include "lsr.h"

int lsr_eql(const ospf_lsr *a, const ospf_lsr *b) {
	return a->ls_type == b->ls_type &&
		   a->ls_id == b->ls_id &&
		   a->ad_router == b->ad_router;
}

void process_lsr(neighbor *nbr, ospf_header *ospfhdr) {
	ospf_lsr *lsr = (ospf_lsr *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	ospf_lsr *last = (ospf_lsr *)((uint8_t *)ospfhdr + ntohs(ospfhdr->length));
	for (;lsr < last; ++lsr) {
		int i = 0;
		for (int i = 0; i < nbr->num_lsr; ++i)
			if (lsr_eql(lsr, nbr->lsrs + i)) break;
		if (i == nbr->num_lsr) nbr->lsrs[nbr->num_lsr++] = *lsr;
	}
}

void produce_lsr(const neighbor *nbr, ospf_header *ospfhdr) {
	ospf_lsr *lsr = (ospf_lsr *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	for (int i = 0; i < nbr->num_lsah; ++i) {
		lsr->ls_type = htonl(nbr->lsahs[i].type);
		lsr->ls_id = nbr->lsahs[i].id;
		lsr->ad_router = nbr->lsahs[i].ad_router;
		++lsr;
	}
	ospfhdr->type = OSPF_TYPE_LSR;
	ospfhdr->length = htons((uint8_t *)lsr - (uint8_t *)ospfhdr);
}
