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

    struct thread* th = (struct thread *)node->obj;

    th->rq_list = NULL;

    return th;
}

static void rt_enqueue(struct thread* thread, struct rq* rq) {
    dlist_insert_last(&thread->rq_node, &rq->rt_rq);

    /* Update the current list */
    thread->rq_list = &rq->rt_rq;
}

static void rt_dequeue(struct thread* thread, struct rq* rq) {
    dlist_remove(&thread->rq_node, &rq->rt_rq);
}

static void rt_block(struct thread* thread, struct rq* rq)
{
    if (thread->rq_list == &rq->blocked_q) {
        return;
    }
    dlist_remove(&thread->rq_node, thread->rq_list); 
    dlist_insert_last(&thread->rq_node, &rq->blocked_q);
    thread->rq_list = &rq->blocked_q;
}

static void rt_unblock(struct thread* thread, struct rq* rq)
{
    if (thread->rq_list != &rq->blocked_q) {
        return;
    }
    dlist_remove(&thread->rq_node, thread->rq_list);
    thread->tick_to_wake = 0;
    dlist_insert_last(&thread->rq_node, &rq->rt_rq);
    thread->rq_list = &rq->rt_rq;
}

/* Real-time scheduling class */
const struct scheduling_class rt_class = {
    .next        = &app_class,
    .pick_thread = rt_pick_thread,
    .enqueue     = rt_enqueue,
    .dequeue     = rt_dequeue,
    .block       = rt_block,
    .unblock     = rt_unblock
};
