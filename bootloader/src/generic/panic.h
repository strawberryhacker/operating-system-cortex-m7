#ifndef PANIC_H
#define PANIC_H

#include "types.h"
#include "debug.h"

#define panic(reason) panic_handler(__FILE__, __LINE__, (reason))

static inline void panic_handler(const char* file_name, u32 line_number,
    const char* reason) {
    
    debug_print("Panic! %s\nFile: %s\nLine: %d\n", reason, file_name,
        line_number);

    asm volatile ("bkpt #0");
}

#endif