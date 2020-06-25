#include "types.h"

extern u32 _got_s;
extern u32 _got_e;
extern u32 _plt_s;
extern u32 _plt_e;
extern u32 _end;

int main(void);

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

    // Prefered scheduler
    enum sched_class scheduler;
};

volatile struct app_info app_info __attribute__((section(".app_info"))) = {
    .end = (u32 *)&(_end),
    .stack = 0x200,
    .name = "Application",
    .scheduler = REAL_TIME,
    .entry = (u32 *)main,

    // Dynmamic linking variables is retrieved from from the linker section
    .got_start = (u32 *)&_got_s,
    .got_end   = (u32 *)&_got_e,
    .plt_start = (u32 *)&_plt_s,
    .plt_end   = (u32 *)&_plt_e
};
