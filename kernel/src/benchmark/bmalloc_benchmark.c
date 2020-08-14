/* Copyright (C) StrawberryHacker */

#include "bmalloc_benchmark.h"
#include "benchmark_timer.h"
#include "clock.h"
#include "nvic.h"
#include "hardware.h"
#include "print.h"
#include "cpu.h"
#include "trand.h"
#include "prand.h"
#include "panic.h"
#include "bmalloc.h"
#include "pmalloc.h"

#include <stddef.h>

#define BMALLOC_BANK BMALLOC_DRAM

enum bmalloc_benchmark_block_status {
    BMALLOC_BM_USED,
    BMALLOC_BM_FREE
};

volatile u32 bmalloc_benchmark_block_pool[BMALLOC_BENCHMARK_BLOCK_COUNT];

/*
 * Clears the block pool list and frees the memory
 */
static void bmalloc_benchmark_clean_pool(void)
{
    for (u32 i = 0; i < BMALLOC_BENCHMARK_BLOCK_COUNT; i++) {
        u32 addr = bmalloc_benchmark_block_pool[i];

        if (addr) {
            bfree((void *)addr);
        }
        bmalloc_benchmark_block_pool[i] = 0;
    }
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

/*
 * Frees a random memory block in the block pool
 */
static u8 bmalloc_benchmark_free_random(u32* time)
{
    u32 index;
    if (!find_block_index(&index, BMALLOC_BM_USED)) {
        return 0;
    }
    benchmark_start_timer();
    bfree((void *)bmalloc_benchmark_block_pool[index]);
    benchmark_stop_timer();
    *time = benchmark_get_us();

    bmalloc_benchmark_block_pool[index] = 0;

    return 1;
}

/*
 * Allocates a block of memory
 */
static u8 bmalloc_benchmark_allocate_random(struct bmalloc_benchmark_conf* conf, u32* time) {

    /* Find a random size */
    u32 free_size = bmalloc_get_free(BMALLOC_BANK);
    u32 size;

    do {
        size = trand() % (conf->max_block_size - conf->min_block_size);
        size += conf->min_block_size;
    } while (size > free_size);

    u32 index;
    if (!find_block_index(&index, BMALLOC_BM_FREE)) {
        return 0;
    }

    /* Try to allocate some memory */
    benchmark_start_timer();
    void* ptr = bmalloc(size, BMALLOC_BANK);
    benchmark_stop_timer();
    *time = benchmark_get_us();

    if (ptr == NULL) {
        return 0;
    }

    bmalloc_benchmark_block_pool[index] = (u32)ptr;

    return 1;
}

void run_bmalloc_benchmark(struct bmalloc_benchmark_conf* conf)
{
    printl("Starting memory manager benchmark");
    benchmark_timer_init();

    for (u32 i = 0; i < BMALLOC_BENCHMARK_BLOCK_COUNT; i++) {
        bmalloc_benchmark_block_pool[i] = 0;
    }

    u32 upper_limit = conf->max_mm_usage * bmalloc_get_total(BMALLOC_BANK) / 100;
    u32 lower_limit = conf->min_mm_usage * bmalloc_get_total(BMALLOC_BANK) / 100;

    print("Upper limit: %d\n", upper_limit);
    print("Lower limit: %d\n", lower_limit);

    /* Allocation and deallocation cound */
    u32 total_alloc_count = 0;
    u32 total_free_count = 0;

    i32 alloc_count = 0;
    i32 free_count = 0;

    u32 alloc_time = 0;
    u32 free_time = 0;

    for (u32 i = 0; i < 1000; i++) {
        alloc_count = 0;
        free_count = 0;

        alloc_time = 0;
        free_time = 0;

        u32 time = 0;

        while (bmalloc_get_used(BMALLOC_BANK) < upper_limit) {
            u8 status = bmalloc_benchmark_allocate_random(conf, &time);
            alloc_time += time;
            if (status == 2) {
                break;
            }
            
            if (status == 0) {
                panic("memory");
            }
            alloc_count++;
            total_alloc_count++;
        }
        
        while (bmalloc_get_used(BMALLOC_BANK) > lower_limit) {
            u8 status = bmalloc_benchmark_free_random(&time);
            free_time += time;
            if (status == 2) {
                break;
            }
            if (status == 0) {
                panic("memory");
            }
            free_count++;
            total_free_count++;
        }
        
        print("Alloc: %d\t - time: %d us\tFree: %d\t - time: %d us\n",
            alloc_count, alloc_time / alloc_count, 
            free_count, free_time / free_count);
    }
    printl("Done - allocs: %d - free: %d\n", total_alloc_count, total_free_count);
}
