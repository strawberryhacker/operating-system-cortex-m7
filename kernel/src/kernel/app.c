#include "app.h"
#include "scheduler.h"

#include <stddef.h>

static struct tcb* app_pick_thread(void) {
    return NULL;
}

static void app_enqueue(struct tcb* thread) {
    
}

static void app_dequeue(struct tcb* thread) {
    
}

static void app_sleep(struct tcb* thread, u32 ms) {
    
}

/// Real-time scheduling class
struct sched_class app_class = {
    .pick_thread = app_pick_thread,
    .enqueue     = app_enqueue,
    .dequeue     = app_dequeue,
    .sleep       = app_sleep 
};
