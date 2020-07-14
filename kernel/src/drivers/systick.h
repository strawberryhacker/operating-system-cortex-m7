/* Copyright (C) StrawberryHacker */

#ifndef SYSTICK_H
#define SYSTICK_H

#include "types.h"
#include "hardware.h"

static inline void systick_set_rvr(u32 value) {
	SYSTICK->RVR = (0xFFFFFF & value);
}

static inline void systick_set_cvr(u32 value) {
	SYSTICK->CVR = (0xFFFFFF & value);
}

static inline u32 systick_get_cvr(void) {
	return SYSTICK->CVR;
}

void systick_enable(u8 irq_enable);

void systick_disable(void);

#endif
