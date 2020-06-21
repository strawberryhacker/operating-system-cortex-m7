/// Copyright (C) StrawberryHacker

#include "rt.h"
#include "scheduler.h"

#include <stddef.h>

static struct thread* rt_pick_thread(struct rq* rq) {
    //printl("RT pick next thread");
    return NULL;
}

static void rt_enqueue(struct thread* thread, struct rq* rq) {
    
}

static void rt_dequeue(struct thread* thread, struct rq* rq) {
    
}

/// Real-time scheduling class
const struct scheduling_class rt_class = {
    .next        = &app_class,
    .pick_thread = rt_pick_thread,
    .enqueue     = rt_enqueue,
    .dequeue     = rt_dequeue
};
