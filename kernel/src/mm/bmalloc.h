/* Copyright (C) StrawberryHacker */

/*
 * bmalloc is the main kernel allocator for allocating small memory units. For
 * bigger units, the user should try to allocate a number of pages instead. This
 * uses pmalloc and is much faster
 */

#ifndef SMALLOC_H
#define SMALLOC_H

#include "types.h"

enum bmalloc_bank {
    BMALLOC_SRAM = 0,
    BMALLOC_DRAM = 4
};

void bfree(void* ptr);

void* bmalloc(u32 count, enum bmalloc_bank bank);
void* bcalloc(u32 size, enum bmalloc_bank bank);

u32 bmalloc_get_used(enum bmalloc_bank bank);
u32 bmalloc_get_free(enum bmalloc_bank bank);
u32 bmalloc_get_total(enum bmalloc_bank bank);

#endif