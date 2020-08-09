/* Copyright (C) StrawberryHacker */

#ifndef SMALLOC_H
#define SMALLOC_H

#include "types.h"

enum smalloc_bank {
    SMALLOC_SRAM,
    SMALLOC_DRAM
};

void* smalloc(u32 count, enum smalloc_bank bank);

void sfree(void* ptr);

u32 smalloc_get_used(enum smalloc_bank bank);

u32 smalloc_get_free(enum smalloc_bank bank);

u32 smalloc_get_total(enum smalloc_bank bank);

#endif