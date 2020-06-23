/// Copyright (C) StrawberryHacker

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"
#include "panic.h"
#include "cpu.h"

struct spinlock {
    u32 lock;
};

static inline u32 strex(volatile u32 *addr, u32 value) {
    uint32_t result;

    asm volatile (
        "strex %0, %2, %1"
        : "=&r" (result), "=Q" (*addr) : "r" (value));
    return result;
}

static inline u32 ldrex(volatile u32 *addr) {
    uint32_t result;

    asm volatile (
        "ldrex %0, %1"
        : "=r" (result) : "Q" (*addr));
    return result;
}

static inline void spinlock_aquire(struct spinlock* spinlock) {
      u32 status = 0;

      do {
          while (ldrex(&spinlock->lock));
          status = strex(&spinlock->lock, 1);
      } while (status);

      dmb();

      if (spinlock->lock != 1) {
        panic("Lock");
    }
}

static inline void spinlock_release(struct spinlock* spinlock) {
    dmb();
    spinlock->lock = 0;
}

#endif