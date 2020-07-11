/// Copyright (C) StrawberryHacker

#ifndef CPU_H
#define CPU_H

#include "types.h"

#define NOINLINE __attribute__((noinline))
#define ALIGN(x) __attribute__((aligned((x))))

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
static inline void cpu_set_basepri(u32 pri) {
	asm volatile ("msr basepri, %0" : : "r"(pri) : "memory");
}

/// Gets the interrupt base priority
static inline u32 cpu_get_basepri(void) {
	u32 basepri;
	asm volatile ("mrs %0, basepri" : "=r"(basepri));
	return basepri;
}

/// Gets the interrupt base priority
static inline u32 cpu_get_psp(void) {
	u32 psp;
	asm volatile ("mrs %0, psp" : "=r"(psp));
	return psp;
}

/// Gets the interrupt base priority
static inline u32 cpu_get_msp(void) {
	u32 msp;
	asm volatile ("mrs %0, msp" : "=r"(msp));
	return msp;
}

/// Functions for getting the processor registers
__attribute__((always_inline)) static inline u32 cpu_get_r0(void) {
    register u32 res;
    asm volatile ("mov %0, r0 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r1(void) {
    register u32 res;
    asm volatile ("mov %0, r1 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r2(void) {
    register u32 res;
    asm volatile ("mov %0, r2 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r3(void) {
    register u32 res;
    asm volatile ("mov %0, r3 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r4(void) {
    register u32 res;
    asm volatile ("mov %0, r4 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r5(void) {
    register u32 res;
    asm volatile ("mov %0, r5 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r6(void) {
    register u32 res;
    asm volatile ("mov %0, r6 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r7(void) {
    register u32 res;
    asm volatile ("mov %0, r7 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r8(void) {
    register u32 res;
    asm volatile ("mov %0, r8 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r9(void) {
    register u32 res;
    asm volatile ("mov %0, r9 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r10(void) {
    register u32 res;
    asm volatile ("mov %0, r10 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r11(void) {
    register u32 res;
    asm volatile ("mov %0, r11 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r12(void) {
    register u32 res;
    asm volatile ("mov %0, r12 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_r13(void) {
    register u32 res;
    asm volatile ("mov %0, r13 \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_lr(void) {
    register u32 res;
    asm volatile ("mov %0, lr \n\t" : "=r" (res));
    return res;
}

__attribute__((always_inline)) static inline u32 cpu_get_pc(void) {
    register u32 res;
    asm volatile ("mov %0, pc \n\t" : "=r" (res));
    return res;
}

#endif
