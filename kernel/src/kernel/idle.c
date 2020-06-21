#include "idle.h"
#include "scheduler.h"
#include "list.h"
#include "thread.h"
#include "panic.h"
#include "print.h"

#include <stddef.h>

struct list rt_rq = { .first = NULL, .size = 0 };
struct list rt_bq = { .first = NULL, .size = 0 };

static struct thread* idle_pick_thread(void) {
    //printl("Idle pick next thread");
    struct list_node* tmp = rt_rq.first;

    if ((struct thread *)tmp->obj == NULL) {
        panic("No idle thread");
    }

    return (struct thread *)tmp->obj;
}

static void idle_enqueue(struct thread* thread) {
    print("IDLE enqueue\n");
    list_insert_first(&thread->rq_node, &rt_rq);
}

static void idle_dequeue(struct thread* thread) {
    
}

static void idle_sleep(struct thread* thread, u32 ms) {
    
}

/// Real-time scheduling class
const struct scheduling_class idle_class = {
    .next        = NULL,
    .pick_thread = idle_pick_thread,
    .enqueue     = idle_enqueue,
    .dequeue     = idle_dequeue,
    .sleep       = idle_sleep 
};
