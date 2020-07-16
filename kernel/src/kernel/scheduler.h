/* Copyright (C) StrawberryHacker */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "list.h"
#include "dlist.h"

#define SYSTICK_RVR 300000
#define THREAD_MAX_NAME_LEN 32

enum sched_class {
    REAL_TIME,
    APPLICATION,
    BACKGROUND,
    IDLE
};

typedef uint32_t tid_t;

/*
 * Info structure used for initializing new threads
 */
struct thread_info {
    char name[THREAD_MAX_NAME_LEN];

    /* Requested stack size in words */
    u32 stack_size;

    /* Function pointer to the thread */
    void (*thread)(void*);

    /* Optional thread argument */
    void* arg;

    enum sched_class class;

    /*
     * Optional code address. If the code is dynamically allocated
     * set this variable to the base address of the code segment
     */
    u32* code_addr;
};

/*
 * Main CPU runqueue structure
 */
struct rq {
    struct dlist app_rq;
    struct dlist background_rq;
    struct dlist rt_rq;
    
    struct thread* idle;

    struct dlist sleep_q;
    struct dlist blocked_q;

    struct dlist threads;

    /*
     * If the sleep queue is non-empty `tick_to_wake` holds the first tick to
     * wake on. Several sleeping threads might have the same tick to wake but 
     * that doesn't matter
     */
    u64 tick_to_wake;
};

/*
 * Main thread control block
 */
struct thread {
    /* The first element in the `tcb` has to be the stack pointer */
    u32* stack_pointer;
    u32* stack_base;

    /* Runqueue list node */
    struct dlist_node rq_node;
    struct dlist_node thread_node;

    /* Pointer to the threads current scheduling class */
    const struct scheduling_class* class;

    /* Name of the thread */
    char name[THREAD_MAX_NAME_LEN];
    u8 name_len;

    u64 tick_to_wake;

    /* Variables used for calulating runtime statistics */
    u64 runtime_curr;
    u64 runtime_new;

    /* Thread ID number */
    tid_t tid;

    /* 
     * Code base address. If this field is zero no dynamic code 
     * is being used and the scheduler does not have to delete
     * the memory.
     */
    u32* code_addr;
};

/*
 * Each scheduling class will have its own set of functions defined
 * in this struct. 
 */
struct scheduling_class {
    /* Link to the next scheduling class */
    const struct scheduling_class* next;

    struct thread* (*pick_thread)(struct rq* rq);
    void           (*enqueue)(struct thread* thread, struct rq* rq);
    void           (*dequeue)(struct thread* thread, struct rq* rq);
};

/*
 * These are the different scheduling classes in the kernel
 */
extern const struct scheduling_class rt_class;
extern const struct scheduling_class app_class;
extern const struct scheduling_class background_class;
extern const struct scheduling_class idle_class;

/*
 * Sets up the interrupts and starts the scheduler
 */
void scheduler_start(void);

void scheduler_enqueue_delay(struct thread* thread);

void reschedule(void);

void suspend_scheduler(void);

void resume_scheduler(void);

u64 get_kernel_tick(void);

u64 get_idle_runtime(void);

#endif