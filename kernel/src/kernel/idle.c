/// Copyright (C) StrawberryHacker

#include "idle.h"
#include "scheduler.h"
#include "list.h"
#include "thread.h"
#include "panic.h"
#include "print.h"

#include <stddef.h>

static struct thread* idle_pick_thread(struct rq* rq) {
    return rq->idle;
}

static void idle_enqueue(struct thread* thread, struct rq* rq) {
    rq->idle = thread;
}

static void idle_dequeue(struct thread* thread, struct rq* rq) {
    
}

/// Idle scheduling class
const struct scheduling_class idle_class = {
    .next        = NULL,
    .pick_thread = idle_pick_thread,
    .enqueue     = idle_enqueue,
    .dequeue     = idle_dequeue
};
