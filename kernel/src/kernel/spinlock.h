/* Copyright (C) StrawberryHacker */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"
#include "panic.h"
#include "cpu.h"
#include "exclusive.h"

struct spinlock {
    u32 lock;
};

static inline void spinlock_init(struct spinlock* spinlock)
{
    spinlock->lock = 0;
}

static inline void spinlock_aquire(struct spinlock* spinlock) {
    u32 status = 0;

    do {
        while (ldrex(&spinlock->lock));
        status = strex(&spinlock->lock, 1);
    } while (status);

    dmb();
}

static inline void spinlock_release(struct spinlock* spinlock) {
    dmb();
    spinlock->lock = 0;
}

#endif