#include "dd.h"

#include <string.h>

#include "common.h"
#include "io.h"
#include "lsdb.h"
#include "nsm.h"


void process_dd(interface *iface,
				struct neighbor *nbr,
				const ospf_header *ospfhdr) {
	ospf_dd * dd = (ospf_dd *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	if (dd->flags & DD_FLAG_I) {
		if (dd->flags & DD_FLAG_M && ntohl(myid) < ntohl(ospfhdr->router_id)) {
			nbr->master = 0;
			nbr->dd_seq_num = dd->seq_num;
			add_event(iface, nbr, E_NegotiationDone);

		}
	} else {
		if (nbr->master) {
			if (dd->seq_num == nbr->dd_seq_num) {
				++nbr->dd_seq_num;
				if (!(dd->flags & DD_FLAG_M))
					add_event(iface, nbr, E_ExchangeDone);
			}
		} else {
			if (dd->seq_num == nbr->dd_seq_num + 1) {
				++nbr->dd_seq_num;
				nbr->more = dd->flags & DD_FLAG_M;
			}
		}
		int num = (ntohs(ospfhdr->length) - sizeof(ospf_header) -
				sizeof(ospf_dd)) / sizeof(lsa_header);
		if (num == 0) nbr->more = 0;
		for (int i = 0; i < num; ++i) {
			if (!find_lsa(areas + iface->area_id, dd->lsahs + i))
				add_lsah(nbr, dd->lsahs +i);
		}
	}
}

void produce_dd(const interface *iface,
				const struct neighbor *nbr,
				ospf_header *ospfhdr) {
	area *const a = areas + iface->area_id;
	ospf_dd *dd = (ospf_dd *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	int num = 0;

	dd->mtu = 1500;
	/* support NSSA */
	dd->options = 0x08;
	dd->flags = 0;
	if (nbr->state == S_ExStart) dd->flags |= DD_FLAG_I | DD_FLAG_M;
	if (nbr->master) dd->flags |= DD_FLAG_MS;
	if (nbr->more) dd->flags |= DD_FLAG_M;
	dd->seq_num = nbr->dd_seq_num;
	if (nbr->state == S_Exchange) {
		for (num = 0; num < a->num_lsa; ++num)
			memcpy(dd->lsahs + num, a->lsas[num], sizeof(lsa_header));
	}
	ospfhdr->type = OSPF_TYPE_DD;
	ospfhdr->length = htons(sizeof(ospf_header) + sizeof(ospf_dd) +
							num * sizeof(lsa_header));
}
