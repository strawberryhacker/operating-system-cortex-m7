/* Copyright (C) StrawberryHacker */

#ifndef GMAC_H
#define GMAC_H

/* 100base-T ethernet chip driver */

#include "types.h"

void gmac_init(void);

void gmac_deinit(void);

void gmac_enable(void);

void gmac_disable(void);

u16 gmac_in_phy(u8 phy_addr, u8 reg);

void gmac_out_phy(u8 phy_addr, u8 reg, u16 data);

void gmac_enable_loop_back(void);

void gmac_disable_loop_back(void);

u32 gmac_get_raw_length(void);

u32 gmac_read_raw(u8* buffer, u32 size);

u32 gmac_write_raw(const u8* buffer, u32 size);

#endif

