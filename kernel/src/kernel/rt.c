/* Copyright (C) StrawberryHacker */

#include "rt.h"
#include "scheduler.h"
#include "dlist.h"

#include <stddef.h>

static struct thread* rt_pick_thread(struct rq* rq) {
    /* Returns the first item in the `rt_rq` */
    if (rq->rt_rq.first == NULL) {
        return NULL;
    }

    struct dlist_node* node = dlist_remove_first(&rq->rt_rq);
    return (struct thread *)node->obj;
}

static void rt_enqueue(struct thread* thread, struct rq* rq) {
    dlist_insert_last(&thread->rq_node, &rq->rt_rq);
}

static void rt_dequeue(struct thread* thread, struct rq* rq) {
    dlist_remove(&thread->rq_node, &rq->rt_rq);
}

/* Real-time scheduling class */
const struct scheduling_class rt_class = {
    .next        = &app_class,
    .pick_thread = rt_pick_thread,
    .enqueue     = rt_enqueue,
    .dequeue     = rt_dequeue
};
