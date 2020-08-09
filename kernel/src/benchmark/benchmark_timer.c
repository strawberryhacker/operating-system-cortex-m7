/* Copyright (C) StrawberryHacker */

#include "benchmark_timer.h"
#include "nvic.h"
#include "clock.h"
#include "hardware.h"

/* Count the number of milliseconds since benchmark_start_timer */
u32 ms_count;

/*
 * Initializing the the timer 1 channel 0. Capture mode, reaload on RC compare,
 * interrupt on RC compare.
 */
void benchmark_timer_init(void)
{
    peripheral_clock_enable(26);

    /* Capture mode with MCK/8 */
    TIMER1->channel[0].CMR = (1 << 14) | (1 << 0);

    /* This gives an interrupt frequency of 1000 Hz */
    TIMER1->channel[0].RC = 18750;

    /* Enable RC compare (overflow) interrupt */
    TIMER1->channel[0].IER = (1 << 4);

    nvic_enable(26);
}

u32 benchmark_get_us(void)
{
    u32 us = TIMER1->channel[0].CV;
    us += 1000 * ms_count;

    return us;
}

void benchmark_start_timer(void)
{
    ms_count = 0;
    TIMER1->channel[0].CCR = 0b101;
}

void benchmark_stop_timer(void)
{
    TIMER1->channel[0].CCR = 0b010;
}

void timer1_ch0_exception(void)
{
    (void)TIMER1->channel[0].SR;
    ms_count++;
}
