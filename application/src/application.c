/// Copyright (C) StrawberryHacker

#include "types.h"

#define NAME       "Print application"
#define SCHEDULER  REAL_TIME
#define STACK_SIZE 200

extern u32 _got_s;
extern u32 _got_e;
extern u32 _plt_s;
extern u32 _plt_e;
extern u32 _end;

enum sched_class {
    REAL_TIME,
    APPLICATION,
    BACKGROUND
};

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
    enum sched_class scheduler;
};

int main(void);

void __libc_init_array(void);

/// Initializes the c library and jumps to the main application
static void app_startup(void* arg) {
    __libc_init_array();

    main();
}

volatile struct app_info app_info __attribute__((section(".app_info"))) = {
    .end       = (u32 *)&(_end),
    .stack     = STACK_SIZE,
    .name      = NAME,
    .scheduler = SCHEDULER,
    .entry     = (u32 *)app_startup,

    .got_start = (u32 *)&_got_s,
    .got_end   = (u32 *)&_got_e,
    .plt_start = (u32 *)&_plt_s,
    .plt_end   = (u32 *)&_plt_e
};


