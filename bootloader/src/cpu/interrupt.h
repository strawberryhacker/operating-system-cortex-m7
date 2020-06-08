#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

/// Enables and disables a subset of the interrupt routines. The `i` flag will
/// prevent execution of all interrupts with a configurable priority. The `f` 
/// flag prevenst execution of all interrupts except the NMI (non-maskable-irq)
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

/// Configures and maintains the NVIC interrupt service
void nvic_enable(u8 irq_number);

void nvic_disable(u8 irq_number);

void nvic_set_prioriy(u8 irq_number);

void nvic_pend(u8 irq_number);

#endif