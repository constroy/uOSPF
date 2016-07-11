#ifndef _LSR_H
#define _LSR_H

#include "packet.h"
#include "structure.h"

void process_lsr(neighbor *nbr, ospf_header *ospfhdr);

void produce_lsr(const neighbor *nbr, ospf_header *ospfhdr);

#endif
