#ifndef GMAC_H
#define GMAC_H

/// Gigabit ethernet chip driver

#include "types.h"

struct gmac_desc {

};

void gmac_init(struct gmac_desc* gmac);

u16 gmac_in_phy(u8 phy_addr, u8 reg);

void gmac_out_phy(u8 phy_addr, u8 reg, u16 data);

#endif