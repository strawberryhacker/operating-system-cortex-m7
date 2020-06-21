/// Copyright (C) StrawberryHacker

#include "background.h"
#include "scheduler.h"
#include "print.h"

#include <stddef.h>

static struct thread* background_pick_thread(void) {
    //printl("Background pick next thread");
    return NULL;
}

static void background_enqueue(struct thread* thread) {
    
}

static void background_dequeue(struct thread* thread) {
    
}

/// Background scheduling class
const struct scheduling_class background_class = {
    .next        = &idle_class,
    .pick_thread = background_pick_thread,
    .enqueue     = background_enqueue,
    .dequeue     = background_dequeue,
    .sleep       = background_sleep 
};
