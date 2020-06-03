/// Copyright (C) StrawberryHacker

#include "types.h"
#include "hardware.h"

static volatile u32 tick = 0;

int main(void) {
	// The processor runs at 12 MHz
	SYSTICK->CSR = 0b111;
	SYSTICK->RVR = 12000;

	GPIOC->PER = (1 << 8);
	GPIOC->OER = (1 << 8);
	GPIOC->CODR = (1 << 8);
	
	asm volatile("cpsie i" : : : "memory");
	
    while (1) {
		if (tick >= 500) {
			tick = 0;
			// Toggle led
			if (GPIOC->ODSR & (1 << 8)) {
				GPIOC->CODR = (1 << 8);
			} else {
				GPIOC->SODR = (1 << 8);
			}
		}
	}
}

void systick_handler() {
	tick++;
}