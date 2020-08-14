/* Copyright (C) StrawberryHacker */

#include "umalloc_benchmark.h"
#include "umalloc.h"
#include "pmalloc.h"
#include "trand.h"
#include "panic.h"
#include "print.h"
#include "benchmark_timer.h"

#include <stddef.h>

#define UMALLOC_SIZE 1000

/* Pointer pool for the bmalloc */
static u32 pointer_pool[UMALLOC_SIZE];

enum umalloc_block_status {
    UMALLOC_BM_USED,
    UMALLOC_BM_FREE
};

static struct umalloc_desc desc;

static void umalloc_benchmark_init(void)
{
    umalloc_new(&desc, 16, UMALLOC_SIZE - 10, PMALLOC_BANK_2);
    benchmark_timer_init();
}

/*
 * Finds a random free memory address in the block pool. Returns 0 if all blocks
 * are allocated i.e. address is zero.
 */
static u8 find_block_index(u32* index, enum umalloc_block_status status)
{
    u32 count = UMALLOC_SIZE;
    u32 pos   = trand() % UMALLOC_SIZE;

    while (count--) {
        if (status == UMALLOC_BM_FREE) {
            if (pointer_pool[pos] == 0x00000000) {
                *index = pos;
                return 1;
            }
        } else {
            if (pointer_pool[pos] != 0x00000000) {
                *index = pos;
                return 1;
            }
        }
        if (pos == 0) {
            pos = UMALLOC_SIZE - 1;
        } else {
            pos--;
        }
    }
    return 0;
}

static u32 umalloc_random(void) 
{
    u32 index = 0;
    if (!find_block_index(&index, UMALLOC_BM_FREE)) {
        panic("List full");
    }
    
    benchmark_start_timer();
    pointer_pool[index] = (u32)umalloc(&desc);
    benchmark_stop_timer();
    u32 return_value = benchmark_get_us();

    if (pointer_pool[index] == 0) {
        panic("Memory full");
    }
    return return_value;
}

static u32 ufree_random(void)
{
    u32 index = 0;
    if (!find_block_index(&index, UMALLOC_BM_USED)) {
        panic("List full");
    }

    benchmark_start_timer();
    ufree(&desc, (u32 *)pointer_pool[index]);
    benchmark_stop_timer();
    u32 return_value = benchmark_get_us();

    pointer_pool[index] = 0x00000000;

    return return_value;
}

void run_umalloc_benchmark(void)
{
    umalloc_benchmark_init();

    u32 count = 0;

    u32 allocate_count = 16;
    u32 free_count = 0;
    while (1) {
        u32 alloc_time = 0;
        for (u32 i = 0; i < allocate_count; ++i) {
            alloc_time += umalloc_random();
        }

        u32 free_time = 0;
        for (u32 i = 0; i < free_count; ++i) {
            free_time += ufree_random();
        }

        print("alloc: %d\tfree: %d cnt: %d\n", alloc_time / allocate_count, 
            free_time / free_count);

        allocate_count += 16;
        free_count += 16;
    }
}