#include "background.h"
#include "scheduler.h"

#include <stddef.h>

static struct tcb* background_pick_thread(void) {
    return NULL;
}

static void background_enqueue(struct tcb* thread) {
    
}

static void background_dequeue(struct tcb* thread) {
    
}

static void background_sleep(struct tcb* thread, u32 ms) {
    
}

/// Real-time scheduling class
struct sched_class background_class = {
    .pick_thread = background_pick_thread,
    .enqueue     = background_enqueue,
    .dequeue     = background_dequeue,
    .sleep       = background_sleep 
};
