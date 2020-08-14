/* Copyright (C) StrawberryHacker */

#include "umalloc.h"
#include "print.h"
#include "panic.h"
#include "memory.h"
#include "umalloc_benchmark.h"
#include <stddef.h>

/*
 * Returns a pointer based on the bitmap index
 */
static inline u32* index_to_addr(struct umalloc_desc* desc, u32 index)
{
    return (u32 *)(desc->arena + (desc->block_size * index));
}

/*
 * Returns the index based on the pointer
 */
static inline u8 addr_to_index(struct umalloc_desc* desc, void* ptr, u32* index)
{
    /* Check the range */
    if (((u32)ptr < (u32)desc->arena) || ((u32)ptr >= (u32)desc->bitmap)) {
        return 0;
    }

    /* Check for unaligned address */
    u32 offset = (u8 *)ptr - desc->arena;
    if (offset % desc->block_size) {
        return 0;
    }

    *index = offset / desc->block_size;
    return 1;
}

/* 
 * Returns the value of the bit in the bitmap the given index
 */
static inline u8 get_bitmap_value(struct umalloc_desc* desc, u32 index)
{
    u32 word_offset = index / 32;
    u32 bit_offset = index % 32;

    if (*(desc->bitmap + word_offset) & (1 << bit_offset)) {
        return 1;
    } else  {
        return 0;
    }
}

/*
 * Clears a bit in the bitmap given a index
 */
static inline void clear_bit_in_bitmap(struct umalloc_desc* desc, u32 index)
{
    u32 word_offset = index / 32;
    u32 bit_offset = index % 32;

    *(desc->bitmap + word_offset) |= (1 << bit_offset);
}

/*
 * Returns the index of the first free block in the bitmap, and marks that bit
 * as used
 */
static inline u8 get_first_free_index(struct umalloc_desc* desc, u32* index)
{
    u32 bitmaps = desc->block_count / 32;
    u32* bitmask_ptr = desc->bitmap;

    for (u32 i = 0; i < bitmaps; ++i) {
        u32 bitmask = *bitmask_ptr;
        if (bitmask) {
            for (u32 j = 0; j < 32; j++) {
                /* A one in the bitmap indicates a free bit  */
                if (bitmask & (1 << j)) {
                    *bitmask_ptr &= ~(1 << j);
                    *index = (32 * i) + j;
                    return 1;
                }
            }
        }
        bitmask_ptr++;
    }
    return 0;
}

/*
 * Configures and allocate a bitmap allocator. This will allign the requested
 * block size to 32 for faster allocation. This uses the pmalloc allocator to
 * dynamically manage the bitmap allocators.
 */
void umalloc_new(struct umalloc_desc* desc, u32 block_size, u32 block_count, 
    enum pmalloc_bank bank)
{
    /* Align the block_count to 32 bit. This makes the search faster */
    block_count = (block_count + 32) & ~(u32)(32 - 1);

    u32 arena_size = block_count * block_size;
    u32 bitmap_size = block_count / 8;
    if (block_count % 8) {
        bitmap_size++;
    }

    /* Find the number of pages to allocate using pmalloc */
    u32 pages = (arena_size + bitmap_size) / 512;
    if ((arena_size + bitmap_size) % 512) {
        pages++;
    }
    desc->arena = (u8 *)pmalloc(pages, bank);

    if (desc->arena == NULL) {
        panic("Can not make ualloc");
    }

    /* Initialize the memory and structure */
    memory_fill(desc->arena, 0xFF, pages * 512);

    desc->block_size = block_size;
    desc->block_count = block_count;
    desc->used_blocks = 0;

    /* Calulate the base address of the bitmap */
    desc->bitmap = (u32 *)(desc->arena + arena_size);
}

/*
 * Deletes the bmalloc allocator. All pointer must be freed before calling this
 * functions. Otherwise, memory leaks will occur. 
 */
void umalloc_delete(struct umalloc_desc* desc)
{
    /* Only one pmalloc allocation has been been performed */
    pfree(desc->arena);
}

static inline void* _umalloc(struct umalloc_desc* desc)
{
    u32 index = 0;
    if (!get_first_free_index(desc, &index)) {
        panic("ups");
        return NULL;
    }
    void* ptr = index_to_addr(desc, index);

    desc->used_blocks++;
    return ptr;
}

/*
 * Allocates a block from the given bitmap allocator
 */
void* umalloc(struct umalloc_desc* desc)
{
    return _umalloc(desc);
}

/*
 * Allocates and zero initializes a block from the given bitmap allocator
 */
void* ucalloc(struct umalloc_desc* desc)
{
    void* ptr = _umalloc(desc);
    memory_fill(ptr, 0x00, desc->block_size);
    return ptr;
}

/*
 * Frees a block in the bmalloc allocator
 */
void ufree(struct umalloc_desc* desc, void* ptr)
{
    u32 index = 0;
    if (!addr_to_index(desc, ptr, &index)) {
        panic("bfree failed");
    }

    /* Check if the index correspondes with a used entry */
    if (get_bitmap_value(desc, index) == 1) {
        panic("bfree failed");
    }
    clear_bit_in_bitmap(desc, index);

    if (desc->used_blocks) {
        desc->used_blocks--;
    } else {
        panic("bfree failed");
    }
}

u32 umalloc_get_used(struct umalloc_desc* desc)
{
    return desc->used_blocks;
}
