#ifndef _HELLO_H
#define _HELLO_H

#include "packet.h"
#include "structure.h"

void process_hello(interface *iface,
				   struct neighbor *nbr,
				   const ospf_header *ospfhdr,
				   in_addr_t src);

void produce_hello(const interface *iface, ospf_header *ospfhdr);

#endif
