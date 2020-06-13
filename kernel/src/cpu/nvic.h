#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"
#include "hardware.h"

static inline void nvic_enable(u8 irq_number) {
    NVIC->ISER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

static inline void nvic_disable(u8 irq_number) {
	NVIC->ICER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

static inline void nvic_set_prioriy(u8 irq_number) {

}

static inline void nvic_pend(u8 irq_number) {

}

static inline void nvic_clear_pending(u8 irq_number) {
	
}

#endif