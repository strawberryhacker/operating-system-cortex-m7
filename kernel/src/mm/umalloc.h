/* Copyright (C) StrawberryHacker */

/*
 * umalloc is the preferred allocator when the user are allocating small blocks
 * of the same size. This will perform up to 50 times faster allocations than
 * pmalloc and smalloc. An example is URBs which is the main target for this
 * allocator
 */

#ifndef BMALLOC_H
#define BMALLOC_H

#include "types.h"
#include "pmalloc.h"

struct umalloc_desc {
    u8* arena;
    u32* bitmap;

    /* All allocation will return this many bytes */
    u32 block_size;
    u32 block_count;

    /* Keeps track of the memory statistics */
    u32 used_blocks;
};

void umalloc_new(struct umalloc_desc* desc, u32 block_size, u32 block_count, 
    enum pmalloc_bank bank);

void umalloc_delete(struct umalloc_desc* desc);

void* umalloc(struct umalloc_desc* desc);

void* ucalloc(struct umalloc_desc* desc);

void ufree(struct umalloc_desc* desc, void* ptr);

u32 umalloc_get_used(struct umalloc_desc* desc);

#endif