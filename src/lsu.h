#ifndef _LSU_H
#define _LSU_H

#include "packet.h"
#include "structure.h"

void process_lsu(area *a, neighbor *nbr, ospf_header *ospfhdr);

void produce_lsu(const area *a, const neighbor *nbr, ospf_header *ospfhdr);

#endif
