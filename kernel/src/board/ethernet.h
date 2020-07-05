/// Copyright (C) StrawberryHacker

#ifndef ETHERNET_H
#define ETHERNET_H

#include "types.h"

void eth_init(void);

void eth_phy_reset_assert(void);

void eth_phy_reset_deassert(void);

#endif
