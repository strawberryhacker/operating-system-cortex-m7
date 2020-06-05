#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

static inline void interrupt_i_enable(void) {
    asm volatile ("cpsie i" : : : "memory");
}

static inline void interrupt_i_disable(void) {
    asm volatile ("cpsid i" : : : "memory");
}

static inline void interrupt_f_enable(void) {
    asm volatile ("cpsie f" : : : "memory");
}

static inline void interrupt_f_disable(void) {
    asm volatile ("cpsid f" : : : "memory");
}

void interrupt_enable(u8 irq_number);

void interrupt_disable(u8 irq_number);

void interrupt_set_prioriy(u8 irq_number);

void interrupt_pend(u8 irq_number);

#endif