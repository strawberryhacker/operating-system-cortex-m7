/// Copyright (C) StrawberryHacker

#include "panic.h"
#include "print.h"

/// If the program enters an undefined state this function will print the 
/// reason and terminate the execution by asserting a breakpoint
void panic_handler(const char* file_name, u32 line_number,
    const char* reason) {
    
    print("Panic! %s \n", reason);
    print("File: %s \n", file_name);
    print("Line: %d \n", line_number);

    print_flush();

    // Halting the processor
    asm volatile ("bkpt #0");
}