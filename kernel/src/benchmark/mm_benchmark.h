/* Copyright (C) StrawberryHacker */

#ifndef MM_BENCHMARK_H
#define MM_BENCHMARK_H

#include "types.h"

/* Defines the number of blocks the mm benchmark can keep track of */
#define SMALLOC_BENCHMARK_BLOCK_COUNT 10000

/* Benchmarking configuration */
struct smalloc_benchmark_conf {
    /* mm_alloc */
    u32 min_block_size;
    u32 max_block_size;

    /* mm_alloc_1k and mm_alloc_4k */
    u32 min_block_count;
    u32 max_block_count;

    /* Memory usage in percent */
    u8 max_mm_usage;
    u8 min_mm_usage;
};

void run_mm_benchmark(struct smalloc_benchmark_conf* conf);

#endif