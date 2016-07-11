#ifndef _LSACK_H
#define _LSACK_H

#include "packet.h"
#include "structure.h"

void process_ack(neighbor *nbr, ospf_header *ospfhdr);

void produce_ack(const neighbor *nbr, ospf_header *ospfhdr);

#endif
