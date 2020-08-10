/* Copyright (C) StrawberryHacker */

#ifndef BMALLOC_H
#define BMALLOC_H

#include "types.h"
#include "pmalloc.h"

/*
 * The bmalloc is used for constant size memory allocations. An example is USB
 * request blocks which are small and must be allocated very fast. This 
 * allocator is based on the pmalloc  which is used to dynamically allocate
 * bitmap allocators. 
 */

struct bmalloc_desc {
    u8* arena_base;
    u8* bitmap_base;

    /* Holds the unit size and the number of units in the arena */
    u32 block_size;
    u32 block_count;

    /* Allocated size in units */
    u32 blocks_used;

    /* Specifies where to allocate the memory from */
    enum pmalloc_bank bank;
};

/*
 * Initializes a bitmap allocator. `desc` is a blank bmalloc descriptor
 */
void bmalloc_init(struct bmalloc_desc* desc, u32 block_count, u32 block_size,
    enum pmalloc_bank bank);

void* bmalloc(struct bmalloc_desc* desc);

void bfree(struct bmalloc_desc* desc, void* ptr);

#endif