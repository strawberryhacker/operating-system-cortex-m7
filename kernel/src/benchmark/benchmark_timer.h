/* Copyright (C) StrawberryHacker */

#ifndef BENCHMARK_TIMER_C
#define BENCHMARK_TIMER_C

#include "types.h"

struct benchmark_time {
    u16 us;
    u16 ms;
    u32 sec;
};

void benchmark_timer_init(void);

u32 benchmark_get_us(void);

void benchmark_start_timer(void);

void benchmark_stop_timer(void);

#endif