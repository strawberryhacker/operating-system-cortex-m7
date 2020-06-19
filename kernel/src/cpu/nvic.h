/// Copyright (C) StrawberryHacker

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"
#include "hardware.h"
#include "cpu.h"

enum irq_priority {
    IRQ_PRIORITY_0,
    IRQ_PRIORITY_1,
    IRQ_PRIORITY_2,
    IRQ_PRIORITY_3,
    IRQ_PRIORITY_4,
    IRQ_PRIORITY_5,
    IRQ_PRIORITY_6,
    IRQ_PRIORITY_7
};

static inline void nvic_enable(u8 irq_number) {
    NVIC->ISER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

static inline void nvic_disable(u8 irq_number) {
	NVIC->ICER[irq_number >> 5] = (1 << (irq_number & 0b11111));
    dsb();
    isb();
}

static inline u8 nvic_is_enabled(u8 irq_number) {
    if (NVIC->ISER[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

static inline void nvic_set_pending(u8 irq_number) {
    NVIC->ISPR[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

static inline void nvic_clear_pending(u8 irq_number) {
	NVIC->ICPR[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

static inline u8 nvic_is_pending(u8 irq_number) {
    if (NVIC->ISPR[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

static inline u8 nvic_is_active(u8 irq_number) {
    if (NVIC->IABR[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

static inline void nvic_set_prioriy(u8 irq_number, enum irq_priority pri) {
    NVIC->IPR[irq_number] = (u8)((pri << 5) & 0xFF);
}

static inline u8 nvic_get_priority(u8 irq_number) {
    return NVIC->IPR[irq_number] >> 5;
}

static inline void systick_set_priority(enum irq_priority pri) {
    SCB->SHPR[11] = (u8)(pri << 5);
}

static inline void pendsv_set_priority(enum irq_priority pri) {
    SCB->SHPR[10] = (u8)(pri << 5);
}

static inline void svc_set_priority(enum irq_priority pri) {
    SCB->SHPR[7] = (u8)(pri << 5);
}

static inline void usage_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[2] = (u8)(pri << 5);
}

static inline void mem_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[0] = (u8)(pri << 5);
}

static inline void bus_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[1] = (u8)(pri << 5);
}

static inline void systick_set_pending(void) {
    SCB->ICSR = (1 << 26);
}

static inline void systick_clear_pending(void) {
    SCB->ICSR = (1 << 25);
}

static inline void pendsv_set_pending(void) {
    SCB->ICSR = (1 << 28);
}

static inline void pendsv_clear_pending(void) {
    SCB->ICSR = (1 << 27);
}

static inline void nmi_set_pending(void) {
    SCB->ICSR = (1 << 31);
}

static inline u8 vector_is_pending(void) {
    if (SCB->ICSR & (1 << 22)) {
        return 1;
    } else {
        return 0;
    }
}

static inline u8 nvic_get_executing_vector(void) {
    return (u8)(SCB->ICSR & 0xFF);
}

#endif
