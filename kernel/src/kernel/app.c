/* Copyright (C) StrawberryHacker */

#include "app.h"
#include "scheduler.h"
#include "print.h"

#include <stddef.h>

static struct thread* app_pick_thread(struct rq* rq) {
    if (rq->app_rq.first == NULL) {
        return NULL;
    }

    struct dlist_node* node = dlist_remove_first(&rq->app_rq);

    return (struct thread *)node->obj;
}

static void app_enqueue(struct thread* thread, struct rq* rq) {
    dlist_insert_last(&thread->rq_node, &rq->app_rq);
}

static void app_dequeue(struct thread* thread, struct rq* rq) {
    dlist_remove(&thread->rq_node, &rq->app_rq);
}

/*
 * Application scheduling class
 */
const struct scheduling_class app_class = {
    .next        = &background_class,
    .pick_thread = app_pick_thread,
    .enqueue     = app_enqueue,
    .dequeue     = app_dequeue
};
