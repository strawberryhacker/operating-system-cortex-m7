/* Copyright (C) StrawberryHacker */

#include "mm_benchmark.h"
#include "benchmark_timer.h"
#include "clock.h"
#include "nvic.h"
#include "hardware.h"
#include "print.h"
#include "cpu.h"
#include "trand.h"
#include "prand.h"

volatile u32 mm_block_pool[MM_BENCHMARK_BLOCK_COUNT];

void run_mm_benchmark(struct mm_benchmark_conf* conf)
{
    printl("Starting memory manager benchmark");
    benchmark_timer_init();

    for (u32 i = 0; i < MM_BENCHMARK_BLOCK_COUNT; i++) {
        mm_block_pool[i] = 0;
    }

    for (u32 i = 0; i < 100; i++) {
        benchmark_start_timer();
        u32 r = trand();
        benchmark_stop_timer();
        print("Trand: %u - \t", r);
        print("us: %d\n", benchmark_get_us());
    }

    for (u32 i = 0; i < 100; i++) {
        benchmark_start_timer();
        u32 r = prand();
        benchmark_stop_timer();
        print("Prand: %u - \t", r);
        print("us: %d\n", benchmark_get_us());
    }
}
