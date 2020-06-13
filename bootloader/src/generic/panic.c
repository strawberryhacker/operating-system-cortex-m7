/// Copyright (C) StrawberryHacker

#include "panic.h"
#include "debug.h"

/// If the program enters an undefined state this function will print the 
/// reason and terminate the execution by asserting a breakpoint
void panic_handler(const char* file_name, u32 line_number,
    const char* reason) {
    
    debug_print("Panic! %s \n", reason);
    debug_print("File: %s \n", file_name);
    debug_print("Line: %d \n", line_number);

    // Halting the processor
    asm volatile ("bkpt #0");
}