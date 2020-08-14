/* Copyright (C) StrawberryHacker */

/*
 * pmalloc is the main kernel allocator. This is used by the thread allocator
 */

#ifndef PMALLOC_H
#define PMALLOC_H

#include "types.h"

enum pmalloc_bank {
    PMALLOC_BANK_1 = 1,
    PMALLOC_BANK_2,
    PMALLOC_BANK_3
};

void pfree(void* ptr);

void* pmalloc(u32 count, enum pmalloc_bank bank);
void* pcalloc(u32 count, enum pmalloc_bank bank);

u32 pmalloc_get_used(enum pmalloc_bank bank);
u32 pmalloc_get_free(enum pmalloc_bank bank);
u32 pmalloc_get_total(enum pmalloc_bank bank);

#endif