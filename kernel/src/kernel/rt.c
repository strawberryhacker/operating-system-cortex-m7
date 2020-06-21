/// Copyright (C) StrawberryHacker

#include "rt.h"
#include "scheduler.h"

#include <stddef.h>

static struct thread* rt_pick_thread(void) {
    //printl("RT pick next thread");
    return NULL;
}

static void rt_enqueue(struct thread* thread) {
    
}

static void rt_dequeue(struct thread* thread) {
    
}

static void rt_sleep(struct thread* thread, u32 ms) {
    
}

/// Real-time scheduling class
const struct scheduling_class rt_class = {
    .next        = &app_class,
    .pick_thread = rt_pick_thread,
    .enqueue     = rt_enqueue,
    .dequeue     = rt_dequeue,
    .sleep       = rt_sleep 
};
