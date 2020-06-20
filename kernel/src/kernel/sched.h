#ifndef SCHED_H
#define SCHED_H

#include "types.h"

struct tcb {
    // The first element in the `tcb` has to be the stack pointer
    u32* stack_pointer;

    u32* stack_base;
};

void sched_start(void);

#endif