#include "nsm.h"

#include <stdio.h>

#include "common.h"
#include "lsdb.h"

const char *state_str[] = {
	"Down",
	"Init",
	"2-Way",
	"ExStart",
	"Exchange",
	"Loading",
	"Full"
};

const char *event_str[] = {
	"HelloReceived",
	"1-WayReceived",
	"2-WayReceived",
	"AdjOK",
	"NegotiationDone",
	"ExchangeDone",
	"LoadingDone"
};

const transition nsm[] = {
	{S_Down,		E_HelloReceived,	S_Init},
	{S_Init,		E_2WayReceived,		S_2Way},
	{S_2Way,		E_1WayReceived,		S_Init},
	{S_2Way,		E_AdjOK,			S_ExStart},
	{S_ExStart,		E_NegotiationDone,	S_Exchange},
	{S_Exchange,	E_ExchangeDone,		S_Loading},
	{S_Loading,		E_LoadingDone,		S_Full}
};

void add_event(interface *iface, struct neighbor *nbr, nbr_event event) {
	const int num = 8;
	puts("--------------------------------");
	printf("State: %s\n", state_str[nbr->state]);
	printf("Event: %s\n", event_str[event]);
	for (int i = 0; i < num; ++i)
		if ((nbr->state == nsm[i].src_state) && (event == nsm[i].event)) {
			nbr->state = nsm[i].dst_state;
			if (nbr->state == S_Full) {
				if (iface->dr == nbr->router_id) iface->state = 1;
				gen_router_lsa(areas + iface->area_id);
			}
			break;
		}
	printf("New State: %s\n", state_str[nbr->state]);
	puts("--------------------------------");             /* 最后状态变化 */
}
