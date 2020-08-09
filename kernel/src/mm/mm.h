/* Copyright (C) StrawberryHacker */

#ifndef MM_H
#define MM_H

#include "types.h"

#define PHYSMEM_NAME_LENGTH 32

enum physmem_e {
    SRAM,
    DRAM_BANK_1,
    DRAM_BANK_2,
    DRAM_BANK_3,
    DRAM_BANK_4
};

struct mm_node {
    struct mm_node* next;

    /*
     * [31..28] - physical memory index
     * [27..0]  - memory block size
     */
    u32 size;
};

struct physmem {
    /* Marks the unaligned start and end address */
    u32 start_addr;
    u32 end_addr;

    /* Currently allocated size and total size */
    u32 size;
    u32 allocated;

    /*
     * Specify the minimum minimum allocation size. This includes
     * 8 bytes for the memory descriptor
     */
    u32 min_alloc;

    /* Contains the name of the memory region */
    char name[PHYSMEM_NAME_LENGTH];

    /*
     * The `root_obj` will contain the first node in the free linked
     * list, and point to the fist node in the physical memory. The
     * `root_node` is pointing to the `root_obj`. The `last_node`
     * will point to the last node in the physical memory, and have
     * size zero.
     */ 
    struct mm_node* root_node;
    struct mm_node* last_node;
    struct mm_node root_obj;
};

void mm_init(void);

void* mm_alloc(u32 size, enum physmem_e index);

void mm_free(void* memory);


u32 mm_get_total(enum physmem_e physmem);

u32 mm_get_used(enum physmem_e physmem);

u32 mm_get_free(enum physmem_e physmem);

u32 mm_get_frag(enum physmem_e physmem);

void mm_check(enum physmem_e physmem);

#endif
