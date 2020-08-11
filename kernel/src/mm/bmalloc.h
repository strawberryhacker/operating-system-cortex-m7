/* Copyright (C) StrawberryHacker */

#ifndef BMALLOC_H
#define BMALLOC_H

#include "types.h"
#include "pmalloc.h"

struct bmalloc_desc {
    u8* arena;
    u32* bitmap;

    /* All allocation will return this many bytes */
    u32 block_size;
    u32 block_count;

    /* Keeps track of the memory statistics */
    u32 used_blocks;
};

void bmalloc_new(struct bmalloc_desc* desc, u32 block_size, u32 block_count, 
    enum pmalloc_bank bank);

void bmalloc_delete(struct bmalloc_desc* desc);

void* bmalloc(struct bmalloc_desc* desc);

void bfree(struct bmalloc_desc* desc, void* ptr);

u32 bmalloc_get_used(struct bmalloc_desc* desc);

#endif