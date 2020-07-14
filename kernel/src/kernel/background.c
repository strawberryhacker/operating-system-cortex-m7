/* Copyright (C) StrawberryHacker */

#include "background.h"
#include "scheduler.h"
#include "print.h"

#include <stddef.h>

static struct thread* background_pick_thread(struct rq* rq) {
    if (rq->background_rq.first == NULL) {
        return NULL;
    }
    
    struct dlist_node* node = dlist_remove_first(&rq->background_rq);

    return (struct thread *)node->obj;
}

static void background_enqueue(struct thread* thread, struct rq* rq) {
    dlist_insert_last(&thread->rq_node, &rq->background_rq);
}

static void background_dequeue(struct thread* thread, struct rq* rq) {
    dlist_remove(&thread->rq_node, &rq->background_rq);
}

/* 
 * Background scheduling class
 */
const struct scheduling_class background_class = {
    .next        = &idle_class,
    .pick_thread = background_pick_thread,
    .enqueue     = background_enqueue,
    .dequeue     = background_dequeue
};
