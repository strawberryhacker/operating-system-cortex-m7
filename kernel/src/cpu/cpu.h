#ifndef CPU_H
#define CPU_H

#include "types.h"

#define __ramfunc__ __attribute__((long_call, section(".ramfunc")))
#define __bootsig__ __attribute__((section(".boot_signature")))
#define __header__ __attribute__((section(".header")))

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

#endif