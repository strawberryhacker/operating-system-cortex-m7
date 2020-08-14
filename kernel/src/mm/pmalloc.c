/* Copyright (C) StrawberryHacker */

#include "pmalloc.h"
#include "mm.h"
#include "memory.h"
#include "panic.h"

#include <stddef.h>

/*
 * Allocates a number pages
 */
void* pmalloc(u32 count, enum pmalloc_bank bank)
{
    void* ptr = mm_alloc(count * 512, (enum physmem_e)bank);

    if (ptr == NULL) {
        panic("palloc failed");
    }
    return ptr;
}

/*
 * Allocates and zero initializes a number of pages
 */
void* pcalloc(u32 count, enum pmalloc_bank bank)
{
    void* ptr = mm_alloc(count * 512, (enum physmem_e)bank);

    if (ptr == NULL) {
        panic("palloc failed");
    }
    memory_fill(ptr, 0x00, count * 512);
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
    return mm_get_used((enum physmem_e)bank);
}

u32 pmalloc_get_free(enum pmalloc_bank bank)
{
    return mm_get_free((enum physmem_e)bank);
}

u32 pmalloc_get_total(enum pmalloc_bank bank)
{
    return mm_get_total((enum physmem_e)bank);
}
