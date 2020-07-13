/// Copyright (C) StrawberryHacker

#include "dynamic_linker.h"
#include "scheduler.h"
#include "thread.h"
#include "memory.h"
#include "print.h"

#include <stddef.h>

static void dynamic_linker(u32* binary);

struct app_info {
    u32* end;
    u32* entry;
    u32 stack;

    // Dynamic linking information
    u32* got_start;
    u32* got_end;
    u32* plt_start;
    u32* plt_end;

    char name[32];

    // Prefered scheduler
    enum sched_class scheduler;
};

/// Dynamically link and run the binary
void dynamic_linker_run(u32* binary) {

    // Get the application information
    struct app_info* app_info = (struct app_info *)binary;
    struct thread_info thread_info;

    thread_info.stack_size = app_info->stack;
    thread_info.arg = NULL;
    thread_info.class = app_info->scheduler;

    // Set the entry point of the binary. Bit number 0 must be set in order
    // to execute in Thumb mode
    u32 entry = (u32)((u8 *)app_info->entry + (u32)binary);
    thread_info.thread = (void (*)(void *))(entry | 0b1);

    memory_copy(app_info->name, thread_info.name, 32);

    // Call the linker in order to relocate the .got and .got.plt sections
    dynamic_linker(binary);

    printl("Launching application: %12s\n", thread_info.name);

    // Make the new thread
    new_thread(&thread_info);
}

static void dynamic_linker(u32* binary) {

    // Get the application information
    struct app_info* info = (struct app_info *)binary;

    // Link the .got section
    u32* got_start = (u32 *)((u8 *)binary + (u32)info->got_start);
    u32* got_end = (u32 *)((u8 *)binary + (u32)info->got_end);

    while (got_start != got_end) {
        *got_start += (u32)binary;
        got_start++;
    }

    // Link the .plt section
    u32* plt_start = (u32 *)((u8 *)binary + (u32)info->plt_start);
    u32* plt_end = (u32 *)((u8 *)binary + (u32)info->plt_end);

    while (plt_start != plt_end) {
        *plt_start += (u32)binary;
        plt_start++;
    }
}
