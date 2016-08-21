#include "dd.h"

#include <string.h>

#include "common.h"
#include "io.h"
#include "lsdb.h"
#include "nsm.h"


void process_dd(interface *iface, neighbor *nbr, const ospf_header *ospfhdr) {
	ospf_dd * dd = (ospf_dd *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	if (dd->flags & DD_FLAG_I) {
		if (dd->flags & DD_FLAG_M && ntohl(myid) < ntohl(ospfhdr->router_id)) {
			nbr->master = 0;
			nbr->dd_seq_num = ntohl(dd->seq_num);
			add_event(iface, nbr, E_NegotiationDone);
		}
	} else {
		if (!(dd->flags & DD_FLAG_MS) && dd->seq_num == htonl(nbr->dd_seq_num)) {
			nbr->master = 1;
			add_event(iface, nbr, E_NegotiationDone);
		}
		if (nbr->master) {
			if (dd->seq_num == htonl(nbr->dd_seq_num)) {
				add_event(iface, nbr, E_NegotiationDone);
				++nbr->dd_seq_num;
				if (!(dd->flags & DD_FLAG_M))
					add_event(iface, nbr, E_ExchangeDone);
			}
		} else {
			if (dd->seq_num == htonl(nbr->dd_seq_num + 1)) {
				++nbr->dd_seq_num;
				nbr->more = dd->flags & DD_FLAG_M;
			}
		}
		int num = (ntohs(ospfhdr->length) - sizeof(ospf_header) -
				sizeof(ospf_dd)) / sizeof(lsa_header);
		if (num == 0) nbr->more = 0;
		for (int i = 0; i < num; ++i) {
			lsa_header *lsah = find_lsa(iface->a, dd->lsahs + i);
			if (!lsah || cmp_lsah(lsah, dd->lsahs + i) < 0)
				add_lsah(nbr, dd->lsahs +i);
		}
	}
}

void produce_dd(const interface *iface, const neighbor *nbr, ospf_header *ospfhdr) {
	ospf_dd *dd = (ospf_dd *)((uint8_t *)ospfhdr + sizeof(ospf_header));
	lsa_header *lsah = dd->lsahs;

	dd->mtu = htons(1500);
	dd->options = OPTIONS;
	dd->flags = 0;
	if (nbr->master) dd->flags |= DD_FLAG_MS;
	if (nbr->state == S_ExStart) dd->flags |= DD_FLAG_I;
	if (nbr->more) dd->flags |= DD_FLAG_M;
	dd->seq_num = htonl(nbr->dd_seq_num);
	if (nbr->state == S_Exchange && nbr->more) {

		for (int i = 0; i < iface->a->num_lsa; ++i)
			memcpy(lsah++, iface->a->lsas[i], sizeof(lsa_header));
	}
	ospfhdr->type = OSPF_TYPE_DD;
	ospfhdr->length = htons((uint8_t *)lsah - (uint8_t *)ospfhdr);
}
