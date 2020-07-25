/* Copyright (C) StrawberryHacker */

#ifndef SYSTICK_H
#define SYSTICK_H

#include "types.h"

void systick_set_rvr(u32 value);

void systick_set_cvr(u32 value);

void systick_enable(u8 irq_enable);

void systick_reset(void);

#endif