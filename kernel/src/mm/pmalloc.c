/* Copyright (C) StrawberryHacker */

#include "pmalloc.h"
#include "mm.h"
#include "panic.h"

#include <stddef.h>

static u8 pmalloc_get_physmem(enum pmalloc_bank bank)
{
    return bank + 1;
}

/*
 * Allocates a number pages using linked list allocator
 */
void* pmalloc(u32 count, enum pmalloc_bank bank)
{
    /* The pmalloc uses the first three DRAM banks */
    enum physmem_e physmem = (enum physmem_e)pmalloc_get_physmem(bank);

    void* ptr = mm_alloc(count * 512, physmem);

    if (ptr == NULL) {
        panic("palloc failed");
    }
    return ptr;
}

/*
 * Free allocated memory from palloc
 */
void pfree(void* ptr)
{
    mm_free(ptr);
}

u32 pmalloc_get_used(enum pmalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)pmalloc_get_physmem(bank);
    return mm_get_used(physmem);
}

u32 pmalloc_get_free(enum pmalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)pmalloc_get_physmem(bank);
    return mm_get_free(physmem);
}

u32 pmalloc_get_total(enum pmalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)pmalloc_get_physmem(bank);
    return mm_get_total(physmem);
}
