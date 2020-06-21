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

struct thread {
    // The first element in the `tcb` has to be the stack pointer
    u32* stack_pointer;
    u32* stack_base;

    // Runqueue list node
    struct list_node rq_node;
};

struct scheduling_class {
    // Link to the next scheduling class
    const struct scheduling_class* next;

    struct thread* (*pick_thread)(void);
    void           (*enqueue)(struct thread* thread);
    void           (*dequeue)(struct thread* thread);
    void           (*sleep)(struct thread* thread, u32 ms);
};

/// These are the different scheduling classes defined in it own files.
extern const struct scheduling_class rt_class;
extern const struct scheduling_class app_class;
extern const struct scheduling_class background_class;
extern const struct scheduling_class idle_class;


void scheduler_start(void);

#endif