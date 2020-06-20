#include "idle.h"
#include "scheduler.h"
#include "list.h"
#include "thread.h"
#include "panic.h"
#include "debug.h"

#include <stddef.h>

struct list rt_rq = { .first = NULL, .size = 0 };
struct list rt_bq = { .first = NULL, .size = 0 };

static struct tcb* idle_pick_thread(void) {
    struct list_node* tmp = rt_rq.first;

    if ((struct tcb *)tmp->obj == NULL) {
        panic("No idle thread");
    }

    return (struct tcb *)tmp->obj;
}

static void idle_enqueue(struct tcb* thread) {
    debug_print("IDLE enqueue\n");
    list_insert_first(&thread->rq_node, &rt_rq);
}

static void idle_dequeue(struct tcb* thread) {
    
}

static void idle_sleep(struct tcb* thread, u32 ms) {
    
}

/// Real-time scheduling class
struct sched_class idle_class = {
    .pick_thread = idle_pick_thread,
    .enqueue     = idle_enqueue,
    .dequeue     = idle_dequeue,
    .sleep       = idle_sleep 
};
