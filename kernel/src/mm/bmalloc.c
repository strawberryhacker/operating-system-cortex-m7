/* Copyright (C) StrawberryHacker */

#include "bmalloc.h"
#include "mm.h"
#include "memory.h"
#include "panic.h"

#include <stddef.h>

/*
 * Allocates a number of bytes
 */
void* bmalloc(u32 size, enum bmalloc_bank bank)
{
    void* ptr = mm_alloc(size, (enum physmem_e)bank);

    if (ptr == NULL) {
        panic("salloc failed");
    }
    return ptr;
}

/*
 * Allocates and zero initialize a number of bytes
 */
void* bcalloc(u32 size, enum bmalloc_bank bank)
{
    void* ptr = mm_alloc(size, (enum physmem_e)bank);

    if (ptr == NULL) {
        panic("salloc failed");
    }
    memory_fill(ptr, 0x00, size);
    return ptr;
}

/*
 * Free allocated memory from palloc
 */
void bfree(void* ptr)
{
    mm_free(ptr);
}

u32 bmalloc_get_used(enum bmalloc_bank bank)
{
    return mm_get_used((enum physmem_e)bank);
}

u32 bmalloc_get_free(enum bmalloc_bank bank)
{
    return mm_get_free((enum physmem_e)bank);
}

u32 bmalloc_get_total(enum bmalloc_bank bank)
{
    return mm_get_total((enum physmem_e)bank);
}
