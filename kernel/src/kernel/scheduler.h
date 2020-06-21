#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "list.h"

struct thread_info {
    char name[32];

    // Requested stack size in words
    u32 stack_size;

    // Function pointer to the thread 
    void (*thread)(void*);

    // Optional thread argument
    void* arg;
};

struct tcb {
    // The first element in the `tcb` has to be the stack pointer
    u32* stack_pointer;
    u32* stack_base;

    // Runqueue list node
    struct list_node rq_node;
};

struct sched_class {
    struct tcb* (*pick_thread)(void);

    void (*enqueue)(struct tcb* thread);
    void (*dequeue)(struct tcb* thread);

    void (*sleep)(struct tcb* thread, u32 ms);
};


void scheduler_start(void);

#endif