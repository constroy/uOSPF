#include "lsr.h"



void produce_lsr(const neighbor *nbr, ospf_header *ospfhdr) {
	ospf_lsr *lsr = (ospf_lsr *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	for (int i = 0; i < nbr->num_lsah; ++i) {
		lsr->ls_type = nbr->lsahs[i].type;
		lsr->ls_id = nbr->lsahs[i].id;
		lsr->ad_router = nbr->lsahs[i].ad_router;
		++lsr;
	}
	ospfhdr->type = OSPF_TYPE_LSR;
	ospfhdr->length = htons((uint8_t *)lsr - (uint8_t *)ospfhdr);
}
