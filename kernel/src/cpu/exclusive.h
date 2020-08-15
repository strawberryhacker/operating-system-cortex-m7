/* Copyright (C) StrawberryHacker */

#ifndef EXCLUSIVE_H
#define EXCLUSIVE_H

#include "types.h"

/*
 * STREX performs a conditional store instruction based on wheather the 
 * exclusive monitor(s) allow it. If the axclusive monitor does not tranck any
 * syncronisation on that address the store will occur and the function will
 * return 0
 */
static inline u32 strex(volatile u32 *addr, u32 value) {
    uint32_t result;

    asm volatile (
        "strex %0, %2, %1"
        : "=&r" (result), "=Q" (*addr) : "r" (value));
    return result;
}

/*
 * LDREX loades the value from the given address and updates the exclusive
 * monitor(s) to tranc the syncronisation. This functions does NOT return any
 * status
 */
static inline u32 ldrex(volatile u32 *addr) {
    uint32_t result;

    asm volatile (
        "ldrex %0, %1"
        : "=r" (result) : "Q" (*addr));
    return result;
}

#endif
