/// Copyright (C) StrawberryHacker

#ifndef CPU_H
#define CPU_H

#include "types.h"

/// Data synchronization barrier
static inline void dsb(void) {
	asm volatile ("dsb sy" : : : "memory");
}

/// Data memory barrier
static inline void dmb(void) {
	asm volatile ("dsb sy" : : : "memory");
}

/// Instructions synchronization barrier
static inline void isb(void) {
	asm volatile ("dsb sy" : : : "memory");
}

/// Enable interrupts with configurable priority
static inline void cpsie_i(void) {
	asm volatile ("cpsie i" : : : "memory");
}

/// Disable interrupts with configurable priority
static inline void cpsid_i(void) {
	asm volatile ("cpsid i" : : : "memory");
}

/// Enable all interrupts except NMI
static inline void cpsie_f(void) {
	asm volatile ("cpsie f" : : : "memory");
}

/// Disable all interrupts except NMI
static inline void cpsid_f(void) {
	asm volatile ("cpsid f" : : : "memory");
}

/// Sets the interrupt base priority
static inline void set_basepri(u32 pri) {
	asm volatile ("msr basepri, %0" : : "r"(pri) : "memory");
}

/// Gets the interrupt base priority
static inline u32 get_basepri(void) {
	u32 basepri;
	asm volatile ("mrs %0, basepri" : "=r"(basepri));
	return basepri;
}

#endif