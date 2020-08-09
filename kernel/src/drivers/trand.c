/* Copyright (C) StrawberryHacker */

#include "trand.h"
#include "hardware.h"
#include "print.h"
#include "clock.h"

void trand_init(void)
{
    peripheral_clock_enable(57);
    TRAND->CR = 0x524E4700 | 0b1;
}

u32 trand(void)
{
    while (!(TRAND->ISR & 0x01));
    return TRAND->ODATA;
}
