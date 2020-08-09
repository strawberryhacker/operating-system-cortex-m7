/* Copyright (C) StrawberryHacker */

#ifndef MM_BENCHMARK_H
#define MM_BENCHMARK_H

#include "types.h"

/* Defines the number of blocks the mm benchmark can keep track of */
#define MM_BENCHMARK_BLOCK_COUNT 1000

/* Benchmarking configuration */
struct mm_benchmark_conf {
    /* The allocation units will be between these values */
    u32 min_block_size;
    u32 max_block_size;

    /* Memory usage in percent */
    u8 max_mm_usage;
    u8 min_mm_usage;
};

void run_mm_benchmark(struct mm_benchmark_conf* conf);

#endif