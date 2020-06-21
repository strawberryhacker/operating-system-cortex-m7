/// Copyright (C) StrawberryHacker

#include "app.h"
#include "scheduler.h"
#include "print.h"

#include <stddef.h>

static struct thread* app_pick_thread(void) {
    //printl("App pick next thread");
    return NULL;
}

static void app_enqueue(struct thread* thread) {
    
}

static void app_dequeue(struct thread* thread) {
    
}

static void app_sleep(struct thread* thread, u32 ms) {
    
}

/// Application scheduling class
const struct scheduling_class app_class = {
    .next        = &background_class,
    .pick_thread = app_pick_thread,
    .enqueue     = app_enqueue,
    .dequeue     = app_dequeue,
    .sleep       = app_sleep 
};
