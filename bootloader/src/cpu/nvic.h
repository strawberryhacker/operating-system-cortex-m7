/* Copyright (C) StrawberryHacker */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"
#include "hardware.h"
#include "cpu.h"

/* The processor only implements 3 priority bits */
enum irq_priority {
	NVIC_PRI_0,
    NVIC_PRI_1,
	NVIC_PRI_2,
	NVIC_PRI_3,
	NVIC_PRI_4,
	NVIC_PRI_5,
	NVIC_PRI_6,
	NVIC_PRI_7
};

/* Enable an NVIC interrupt vector */
static inline void nvic_enable(u8 irq_number) {
    NVIC->ISER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

/* Disable an NVIC interrupt vector */
static inline void nvic_disable(u8 irq_number) {
	NVIC->ICER[irq_number >> 5] = (1 << (irq_number & 0b11111));
    dsb();
    isb();
}

/* Checks whether an NVIC interrupt vector is enabled */
static inline u8 nvic_is_enabled(u8 irq_number) {
    if (NVIC->ISER[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

/* Pends the execution of an NVIC interrupt vector */
static inline void nvic_set_pending(u8 irq_number) {
    NVIC->ISPR[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

/* Clear the pending flag of an NVIC interrupt vector */
static inline void nvic_clear_pending(u8 irq_number) {
	NVIC->ICPR[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

/* Checks whether an NVIC interrupt vector is pending */
static inline u8 nvic_is_pending(u8 irq_number) {
    if (NVIC->ISPR[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

/* Checks whether an NVIC interrupt vector is active or active and pending */
static inline u8 nvic_is_active(u8 irq_number) {
    if (NVIC->IABR[irq_number >> 5] & (1 << (irq_number & 0b11111))) {
        return 1;
    } else {
        return 0;
    }
}

/* Sets the priority of an NVIC interrupt vector */
static inline void nvic_set_prioriy(u8 irq_number, enum irq_priority pri) {
    NVIC->IPR[irq_number] = (u8)((pri << 5) & 0xFF);
}

/* Gets the priority of an NVIC interrupt vector */
static inline u8 nvic_get_priority(u8 irq_number) {
    return NVIC->IPR[irq_number] >> 5;
}

/* Sets the priority of the systick interrupt vector */
static inline void systick_set_priority(enum irq_priority pri) {
    SCB->SHPR[11] = (u8)(pri << 5);
}

/* Sets the priority of the pendsv interrupt vector */
static inline void pendsv_set_priority(enum irq_priority pri) {
    SCB->SHPR[10] = (u8)(pri << 5);
}

/* Sets the priority of the svc interrupt vector */
static inline void svc_set_priority(enum irq_priority pri) {
    SCB->SHPR[7] = (u8)(pri << 5);
}

/* Sets the priority of the usage fault interrupt vector */
static inline void usage_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[2] = (u8)(pri << 5);
}

/* Sets the priority of the memory fault interrupt vector */
static inline void mem_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[0] = (u8)(pri << 5);
}

/* Sets the priority of the bus fault interrupt vector */
static inline void bus_fault_set_priority(enum irq_priority pri) {
    SCB->SHPR[1] = (u8)(pri << 5);
}

/* Pend the execution of the systick handler */
static inline void systick_set_pending(void) {
    SCB->ICSR = (1 << 26);
}

/* Clear the pending flag of the systick handler */
static inline void systick_clear_pending(void) {
    SCB->ICSR = (1 << 25);
}

/* Pend the execution of the pendsv handler */
static inline void pendsv_set_pending(void) {
    SCB->ICSR = (1 << 28);
}

/* Clear the pending flag of the pendsv handler */
static inline void pendsv_clear_pending(void) {
    SCB->ICSR = (1 << 27);
}

/* Pend the execution of the NMI handler */
static inline void nmi_set_pending(void) {
    SCB->ICSR = (1 << 31);
}

/* Check if a interrupt vector is pending */
static inline u8 vector_is_pending(void) {
    if (SCB->ICSR & (1 << 22)) {
        return 1;
    } else {
        return 0;
    }
}

/* Returns the vector number of the current execution vector */
static inline u8 nvic_get_executing_vector(void) {
    return (u8)(SCB->ICSR & 0xFF);
}

#endif
