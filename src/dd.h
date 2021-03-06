#ifndef _DD_H
#define _DD_H

#include "packet.h"
#include "structure.h"

void process_dd(interface *iface, neighbor *nbr, const ospf_header *ospfhdr);

void produce_dd(const interface *iface, const neighbor *nbr, ospf_header *ospfhdr);

#endif
