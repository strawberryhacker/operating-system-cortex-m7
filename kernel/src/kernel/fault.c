/// Copyright (C) StrawberryHacker

#include "panic.h"
#include "gpio.h"
#include "print.h"
#include "cpu.h"

void mem_fault(void) {
    gpio_clear(GPIOC, 8);
    panic("Memory fault");
}

void bus_fault(void) {
    gpio_clear(GPIOC, 8);
    panic("Bus fault");
}

void usage_fault(void) {
    gpio_clear(GPIOC, 8);
    panic("Usage fault");
}

void hard_fault(u32* stack_pointer) {
    gpio_clear(GPIOC, 8);
    printl("Stack pointer: 0x%4h", stack_pointer);
    printl("PC: 0x%4h", stack_pointer[6]);
    printl("R0: %4h", cpu_get_r0());
    printl("R1: %4h", cpu_get_r1());
    printl("R2: %4h", cpu_get_r2());
    printl("R3: %4h", cpu_get_r3());
    printl("R4: %4h", cpu_get_r4());
    printl("R5: %4h", cpu_get_r5());
    printl("R6: %4h", cpu_get_r6());
    printl("R7: %4h", cpu_get_r7());
    printl("R8: %4h", cpu_get_r8());
    printl("R9: %4h", cpu_get_r9());
    printl("R10: %4h", cpu_get_r10());
    printl("R11: %4h", cpu_get_r11());
    printl("R12: %4h", cpu_get_r12());
    printl("R13: %4h", cpu_get_r13());
    printl("LR: %4h", cpu_get_lr());
    printl("PC: %4h", cpu_get_pc());
    panic("Hard fault");
}