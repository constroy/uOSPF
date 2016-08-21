#ifndef _IO_H
#define _IO_H

#include "common.h"
#include "packet.h"

void if_init();

interface *recv_ospf(int sock, uint8_t buf[], int size, in_addr_t *src);

void send_ospf(const interface *iface, struct iphdr *iph, in_addr_t dst);

#endif
