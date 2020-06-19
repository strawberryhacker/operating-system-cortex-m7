/// Copyright (C) StrawberryHacker

#include "systick.h"
#include "hardware.h"

void systick_set_rvr(u32 value) {
	SYSTICK->RVR = (0xFFFFFF & value);
}

void systick_set_cvr(u32 value) {
	SYSTICK->CVR = (0xFFFFFF & value);
}

void systick_enable(u8 irq_enable) {
	// Enable flag and processor clock source flag
	u32 reg = 0b101;
	
	if (irq_enable) {
		reg |= (1 << 1);
	}
	
	SYSTICK->CSR = reg;
}

void systick_reset(void) {
	SYSTICK->CSR = 0;
}
