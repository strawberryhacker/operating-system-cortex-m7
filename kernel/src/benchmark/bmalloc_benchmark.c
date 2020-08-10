/* Copyright (C) StrawberryHacker */

#include "smalloc_benchmark.h"
#include "benchmark_timer.h"
#include "trand.h"
#include "bmalloc.h"
#include "pmalloc.h"
#include "memory.h"
#include "panic.h"

#define TEST_SIZE 43
#define TEST_BANK PMALLOC_BANK_2
#define BMALLOC_BENCHMARK_BLOCK_COUNT 10000

enum bmalloc_benchmark_block_status {
    BMALLOC_BM_USED,
    BMALLOC_BM_FREE
};

struct bmalloc_desc test_allocator;
static volatile u32 bmalloc_benchmark_block_pool[BMALLOC_BENCHMARK_BLOCK_COUNT];

static void bmalloc_bechmark_init(void)
{
    bmalloc_init(&test_allocator, 64, TEST_SIZE, TEST_BANK);
    print("Base: %4h\n", test_allocator.arena_base);
    memory_fill(bmalloc_benchmark_block_pool, 0x00, BMALLOC_BENCHMARK_BLOCK_COUNT * 4);
}

/*
 * Finds a random free memory address in the block pool. Returns 0 if all blocks
 * are allocated i.e. address is zero.
 */
static u8 find_block_index(u32* index, enum bmalloc_benchmark_block_status status)
{
    u32 count = BMALLOC_BENCHMARK_BLOCK_COUNT;
    u32 pos   = trand() % BMALLOC_BENCHMARK_BLOCK_COUNT;

    while (count--) {
        if (status == BMALLOC_BM_FREE) {
            if (bmalloc_benchmark_block_pool[pos] == 0x00000000) {
                *index = pos;
                return 1;
            }
        } else {
            if (bmalloc_benchmark_block_pool[pos] != 0x00000000) {
                *index = pos;
                return 1;
            }
        }
        if (pos == 0) {
            pos = BMALLOC_BENCHMARK_BLOCK_COUNT - 1;
        } else {
            pos--;
        }
    }
    return 0;
}

void allocate_random(void)
{
    u32 index;
    if (!find_block_index(&index, BMALLOC_BM_FREE)) {
        panic("ouf");
    }
    u32 ptr = (u32)bmalloc(&test_allocator);
    if (!ptr) {
        panic("o8f");
    }
    bmalloc_benchmark_block_pool[index] = ptr;
}

void free_random(void)
{
    u32 index;
    if (!find_block_index(&index, BMALLOC_BM_USED)) {
        panic("ouf");
    }
    bfree(&test_allocator, (void *)bmalloc_benchmark_block_pool[index]);

    bmalloc_benchmark_block_pool[index] = 0;
}

void run_bmalloc_benchmark(void)
{
    bmalloc_bechmark_init();
    for (u32 r = 0; r < 500; r++) {
        for (u32 i = 0; i < 50 + r; i++) {
            allocate_random();
        }
        for (u32 i = 0; i < 50; i++) {
            free_random();
        }
    }
}