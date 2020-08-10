/* Copyright (C) StrawberryHacker */

#include "bmalloc.h"
#include "print.h"
#include "panic.h"
#include "memory.h"
#include <stddef.h>

static inline u32 bits_to_pages(u32 bit_count)
{
    u32 bytes = bit_count / 8;
    if (bit_count % 8) {
        bytes++;
    }
    u32 pages = bytes / 512;
    if (bytes % 512) {
        pages++;
    }
    return pages;
}

static inline u32 blocks_to_pages(u32 block_count, u32 block_size)
{
    u32 bytes = block_count * block_size;
    u32 pages = bytes / 512;
    if (bytes % 512) {
        pages++;
    }
    return pages;
}

/*
 * Extends the bitmap buffer to the given number of pages
 */
static u8 extend_bitmap(struct bmalloc_desc* desc, u32 pages)
{
    print("Bitmap: allocating %d pages\n", pages);
    u8* ptr = (u8 *)pmalloc(pages, desc->bank);
    if (ptr == NULL) {
        return 0;
    }
    memory_fill(ptr, 0x00, pages * 512);

    /* Copy the bitmask data */
    u32 bytes = desc->block_count / 8;
    if (desc->block_count % 8) {
        bytes++;
    }
    print("Bitmap: bytes to copy: %d\n", bytes);
    memory_copy(desc->bitmap_base, ptr, bytes);

    /* Delete and switch */
    if (desc->bitmap_base) {
        pfree(desc->bitmap_base);
    }
    desc->bitmap_base = ptr;

    return 1;
}

/*
 * Extends the arena buffer to the given number of pages
 */
static u8 extend_arena(struct bmalloc_desc* desc, u32 pages)
{
    print("Arena: allocating %d pages\n", pages);
    u8* ptr = (u8 *)pmalloc(pages, desc->bank);
    if (ptr == NULL) {
        return 0;
    }
    memory_fill(ptr, 0x00, pages * 512);

    /* Copy the bitmask data */
    u32 bytes = desc->block_count * desc->block_size;
    print("Arena: bytes to copy %d\n", bytes);
    memory_copy(desc->bitmap_base, ptr, bytes);

    /* Delete and switch */
    if (desc->arena_base) {
        pfree(desc->arena_base);
    }
    desc->arena_base = ptr;

    return 1;
}

/*
 * Doubles the capacity of a bitmap allocator
 */
static u8 bmalloc_extend(struct bmalloc_desc* desc)
{
    u32 block_count = desc->block_count * 2;

    u32 prev_page_cnt = blocks_to_pages(desc->block_count, desc->block_size);
    u32 curr_page_cnt = blocks_to_pages(block_count, desc->block_size);

    /* Check if the arena must be extended */
    if (curr_page_cnt > prev_page_cnt) {
        if (!extend_arena(desc, curr_page_cnt)) {
            return 0;
        }
    }

    prev_page_cnt = bits_to_pages(desc->block_count);
    curr_page_cnt = bits_to_pages(block_count);

    /* Check if the bitmap must be extended */
    if (curr_page_cnt > prev_page_cnt) {
        if (!extend_bitmap(desc, curr_page_cnt)) {
            return 0;
        }
    }
    /* Update the block count */
    desc->block_count = block_count;
    return 1;
}

/*
 * Returns a pointer to arena block with the given index
 */
static u8* get_arena_pointer(struct bmalloc_desc* desc, u32 index)
{
    print("Getting pointer %d\n", index);
    u32 offset = index * desc->block_size;
    return (desc->arena_base + offset);
}

/*
 * Returns the bitmap index based on the arena pointer
 */
static i32 get_bitmap_index(struct bmalloc_desc* desc, void* ptr)
{
    /* Check that the pointer belongs to the bmalloc unit */
    u32 size = desc->block_size * desc->block_count;
    if ((ptr < desc->arena_base) || (ptr > (desc->arena_base + size))) {
        print("BASE: %4h SIZE: %d PTR: %4h\n", desc->arena_base, size, ptr);
        panic("Erro");
        return -1;
    }
    u32 offset = (u32)ptr - (u32)desc->arena_base;
    if (offset % desc->block_size) {
        panic("Unaligned free");
        return -1;
    }
    return (offset / desc->block_size);
}

/*
 * Find the first free block and returns the index to that block. Returns -1 if
 * no block is free
 */
static i32 get_free_index(struct bmalloc_desc* desc)
{
    u32 block_count = desc->block_count;
    u32* bitmap_iterator = (u32 *)desc->bitmap_base;

    print("Block count: %d\n", desc->block_count);
    for (u32 i = 0; i < desc->block_count; i += 32) {
        u32 bitmask = *bitmap_iterator;

        /* Bit is zero for free block and one for a used block */
        if (bitmask != 0xFFFFFFFF) {
            for (u32 j = 0; j < 32; j++) {

                /*
                 * Due to variable sized blocks there is no guarantee that a 
                 * multiple of 32 block fits inside a page
                 */
                if ((i + j) >= desc->block_count) {
                    return -1;
                }
                if (!(bitmask & (1 << j))) {
                    *bitmap_iterator |= (1 << j);
                    print("i: %d j: %d\n", i, j);
                    return (i + j);
                }
            }
        }
        bitmap_iterator++;
    }
    return -1;
}

/*
 * Initializes a bitmap allocator. `desc` is a blank bmalloc descriptor
 */
void bmalloc_init(struct bmalloc_desc* desc, u32 block_count, u32 block_size,
    enum pmalloc_bank bank)
{
    /* Initialize the allocator */
    desc->block_count = 0;
    desc->blocks_used = 0;

    desc->bitmap_base = NULL;
    desc->arena_base = NULL;

    desc->block_size = block_size;
    desc->bank = bank;

    /* Allocate the arena */
    u32 pages = blocks_to_pages(block_count, desc->block_size);
    if (!extend_arena(desc, pages)) {
        panic("Cant allocate arena");
    }

    /* Allocate the bitmap */
    pages = bits_to_pages(block_count);
    if (!extend_bitmap(desc, pages)) {
        panic("Cant allocate bitmap");
    }

    /* Update the total block count */
    desc->block_count = block_count;
}

void* bmalloc(struct bmalloc_desc* desc)
{
    /* Find a free index */
    i32 index = get_free_index(desc);

    print("Allocating index: %d\n", index);

    /*
     * Check if the buffer is full. In case of a full buffer the pmalloc is 
     * used to double the allocated bmalloc size
     */
    if (index < 0) {
        print("Buffer is full?\n");
        if (!bmalloc_extend(desc)) {
            panic("Cant extend bmalloc");
        }
        print("desc: block_count: %d\n", desc->block_count);
        index = get_free_index(desc);

        if (index < 0) {
            panic("Back to school");
        }
    }

    return (void *)get_arena_pointer(desc, index);
}

void bfree(struct bmalloc_desc* desc, void* ptr)
{
    i32 bit_offset = get_bitmap_index(desc, ptr);
    if (bit_offset < 0) {
        panic("ok");
    }
    u32 byte_offset = (u32)(bit_offset / 8);
    bit_offset = (bit_offset % 8);

    print("Freeing index: bit %d byte %d\n", bit_offset, byte_offset);
    desc->bitmap_base[byte_offset] |= (1 << bit_offset);
}
