#include "rt.h"
#include "scheduler.h"

#include <stddef.h>

static struct tcb* rt_pick_thread(void) {
    return NULL;
}

static void rt_enqueue(struct tcb* thread) {
    
}

static void rt_dequeue(struct tcb* thread) {
    
}

static void rt_sleep(struct tcb* thread, u32 ms) {
    
}

/// Real-time scheduling class
struct sched_class rt_class = {
    .pick_thread = rt_pick_thread,
    .enqueue     = rt_enqueue,
    .dequeue     = rt_dequeue,
    .sleep       = rt_sleep 
};
