/* Copyright (C) StrawberryHacker */

#include "panic.h"
#include "print.h"
#include "sections.h"
#include "memory.h"
#include "cpu.h"
#include "cache.h"

/*
 * This is declared in the `bootloader.h`
 */
extern __bootsig__ u8 boot_signature[32];

/*
 * If the program enters an undefined state this function will print
 * the reason and terminate the execution by going to the bootloader
 */
void panic_handler(const char* file_name, u32 line_number, const char* reason) {
    
    cpsid_f();

    dcache_disable();
    icache_disable();
    
    print("Panic! %s \n", reason);
    print("File: %s \n", file_name);
    print("Line: %d \n", line_number);
    print_flush();

    /*
     * The panic is used when the system is in an unrecoverable state
     * or should not proceed. The flash image is still "valid" so the
     * bootloader will run it when it starts. To prevent that, the
     * panic will copy the boot signature into the boot signature area
     */
    memory_copy("StayInBootloader", boot_signature, 16);
    dmb();
    
    /* Perform a soft reset */
    *((u32 *)0x400E1800) = 0xA5000000 | 0b1;

    while (1);
}
