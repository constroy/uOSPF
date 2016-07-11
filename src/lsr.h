#ifndef _LSR_H
#define _LSR_H

#include "packet.h"
#include "structure.h"


void produce_lsr(const neighbor *nbr, ospf_header *ospfhdr);

#endif
