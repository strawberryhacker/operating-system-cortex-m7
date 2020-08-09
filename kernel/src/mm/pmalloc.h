/* Copyright (C) StrawberryHacker */

#ifndef PMALLOC_H
#define PMALLOC_H

#include "types.h"

enum pmalloc_bank {
    PMALLOC_BANK_1,
    PMALLOC_BANK_2,
    PMALLOC_BANK_3
};

void* pmalloc(u32 count, enum pmalloc_bank bank);

void pfree(void* ptr);

u32 pmalloc_get_used(enum pmalloc_bank bank);

u32 pmalloc_get_free(enum pmalloc_bank bank);

u32 pmalloc_get_total(enum pmalloc_bank bank);

#endif