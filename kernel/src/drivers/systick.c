/* Copyright (C) StrawberryHacker */

#include "systick.h"

void systick_enable(u8 irq_enable) {
	/* Enable flag and processor clock source flag */
	u32 reg = 0b101;
	
	if (irq_enable) {
		reg |= (1 << 1);
	}
	
	SYSTICK->CSR = reg;
}

void systick_disable(void) {
	SYSTICK->CSR = 0;
}
