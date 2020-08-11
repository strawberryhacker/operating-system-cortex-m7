/* Copyright (C) StrawberryHacker */

#include "bmalloc.h"
#include "print.h"
#include "panic.h"
#include "memory.h"
#include <stddef.h>

/*
 * Returns a pointer based on the bitmap index
 */
static u32* index_to_addr(struct bmalloc_desc* desc, u32 index)
{
    return (u32 *)(desc->arena + (desc->block_size * index));
}

/*
 * Returns the index based on the pointer
 */
static u8 addr_to_index(struct bmalloc_desc* desc, void* ptr, u32* index)
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
static u8 get_bitmap_value(struct bmalloc_desc* desc, u32 index)
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
static void clear_bit_in_bitmap(struct bmalloc_desc* desc, u32 index)
{
    u32 word_offset = index / 32;
    u32 bit_offset = index % 32;

    *(desc->bitmap + word_offset) &= ~(1 << bit_offset);
}

/*
 * Returns the index of the first free block in the bitmap, and marks that bit
 * as used
 */
static u8 get_first_free_index(struct bmalloc_desc* desc, u32* index)
{
    for (u32 i = 0; i < desc->block_count; i += 32) {
        u32 bitmask = desc->bitmap[i / 32];

        if (bitmask != 0xFFFFFFFF) {
            /* The the bitmap for the free bit */
            for (u32 j = 0; j < 32; j++) {

                /* 
                 * The block count does not nessecarily have to be a multiple 
                 * of 32 bits
                 */
                if ((i + j) >= desc->block_count) {
                    return 0;
                }

                /* Check if a bit is free */
                if (!(bitmask & (1 << j))) {
                    desc->bitmap[i / 32] |= (1 << j);
                    *index = i + j;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * Configures and allocate a bitmap allocator
 */
void bmalloc_new(struct bmalloc_desc* desc, u32 block_size, u32 block_count, 
    enum pmalloc_bank bank)
{
    /*
     * Only one block is allocated from the pmalloc and is shared between the 
     * arena and the bitmap
     */
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

    /* Initialize the memory and structure */
    memory_fill(desc->arena, 0x00, pages * 512);

    desc->block_size = block_size;
    desc->block_count = block_count;
    desc->used_blocks = 0;

    /* Calulate the base address of the bitmap */
    desc->bitmap = (u32 *)(desc->arena + arena_size);

    print("Arena base address: %4h\n", desc->arena);
    print("Bitmap base address: %4h\n", desc->bitmap);

    print("Block count: %d\n", desc->block_count);
}

/*
 * Deletes the bmalloc allocator
 */
void bmalloc_delete(struct bmalloc_desc* desc)
{
    /* Only one pmalloc allocation has been been performed */
    pfree(desc->arena);
}

/*
 * Allocates a block from the given bitmap allocator
 */
void* bmalloc(struct bmalloc_desc* desc)
{
    u32 index = 0;
    if (!get_first_free_index(desc, &index)) {
        panic("ups");
        return NULL;
    }
    void* ptr = index_to_addr(desc, index);

    if (ptr == NULL) {
        panic("Ups");
    }
    desc->used_blocks++;
    return ptr;
}

/*
 * Frees a block in the bmalloc allocator
 */
void bfree(struct bmalloc_desc* desc, void* ptr)
{
    u32 index = 0;
    if (!addr_to_index(desc, ptr, &index)) {
        panic("bfree failed");
    }

    /* Check if the index correspondes with a used entry */
    if (get_bitmap_value(desc, index) == 0) {
        panic("bfree failed");
    }

    clear_bit_in_bitmap(desc, index);
    if (desc->used_blocks) {
        desc->used_blocks--;
    } else {
        panic("bfree failed");
    }
}

u32 bmalloc_get_used(struct bmalloc_desc* desc)
{
    return desc->used_blocks;
}
