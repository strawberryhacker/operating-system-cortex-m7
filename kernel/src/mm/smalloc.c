/* Copyright (C) StrawberryHacker */

#include "smalloc.h"
#include "mm.h"
#include "panic.h"

#include <stddef.h>

static u8 smalloc_get_physmem(enum smalloc_bank bank)
{
    if (bank == SMALLOC_SRAM) {
        return SRAM;
    } else {
        return DRAM_BANK_4;
    }
}

/*
 * Allocates a number pages using linked list allocator
 */
void* smalloc(u32 size, enum smalloc_bank bank)
{
    /* The smalloc uses the fourth DRAM bank and the internal SRAM */
    enum physmem_e physmem = (enum physmem_e)smalloc_get_physmem(bank);

    void* ptr = mm_alloc(size, physmem);

    if (ptr == NULL) {
        panic("salloc failed");
    }
    return ptr;
}

/*
 * Free allocated memory from palloc
 */
void sfree(void* ptr)
{
    mm_free(ptr);
}

u32 smalloc_get_used(enum smalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)smalloc_get_physmem(bank);
    return mm_get_used(physmem);
}

u32 smalloc_get_free(enum smalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)smalloc_get_physmem(bank);
    return mm_get_free(physmem);
}

u32 smalloc_get_total(enum smalloc_bank bank)
{
    enum physmem_e physmem = (enum physmem_e)smalloc_get_physmem(bank);
    return mm_get_total(physmem);
}
