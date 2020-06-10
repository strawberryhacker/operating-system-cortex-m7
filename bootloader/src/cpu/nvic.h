#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

void nvic_enable(u8 irq_number);

void nvic_disable(u8 irq_number);

void nvic_set_prioriy(u8 irq_number);

void nvic_pend(u8 irq_number);

void nvic_clear_pending(u8 irq_number);

#endif