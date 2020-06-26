/// Copyright (C) StrawberryHacker

#include "panic.h"
#include "gpio.h"
#include "print.h"
#include "cpu.h"

static inline __attribute__((always_inline)) void print_bus_fault(void) {
    volatile u32 bus_status = *(volatile u32 *)0xE000ED29;

    if (bus_status & 0xFF) {
        printl("Bus fault has occured: ");
    }

    if (bus_status & (1 << 7)) {
        printl("Bus fault address known");
    } else if (bus_status & (1 << 5)) {
        printl("Bus fault during FPU lazy state preservation");
    } else if (bus_status & (1 << 4)) {
        printl("Exception stacking caused a bus fault");
    } else if (bus_status & (1 << 3)) {
        printl("Exception unstacking caused a bus fault");
    } else if (bus_status & (1 << 2)) {
        printl("Imprecise bus fault");
    } else if (bus_status & (1 << 1)) {
        printl("Precise error - PC contains the reason");
    } else if (bus_status & (1 << 0)) {
        printl("Instruction bus error");
    }
}

static inline __attribute__((always_inline)) void print_usage_fault(void) {
    volatile u32 usage_status = *(volatile u32 *)0xE000ED2A;

    if (usage_status & 0xFFFF) {
        printl("Usage fault has occured: ");
    }

    if (usage_status & (1 << 9)) {
        printl("Devision by zero");
    } else if (usage_status & (1 << 8)) {
        printl("Unaligned memory access");
    } else if (usage_status & (1 << 3)) {
        printl("Coprocessor error - instruction not supported");
    } else if (usage_status & (1 << 2)) {
        printl("Illegal use of EXE_RETURN");
    } else if (usage_status & (1 << 1)) {
        printl("Illegal use of ePSR");
    } else if (usage_status & (1 << 0)) {
        printl("Undefined instruction");
    }
}

static inline __attribute__((always_inline)) void print_memory_fault(void) {
    volatile u32 mem_status = *(volatile u32 *)0xE000ED28;

    if (mem_status & 0xFF) {
        printl("Memory fault has occured:");
    }

    if (mem_status & (1 << 7)) {
        printl("MMAR hold the fault address");
    } else if (mem_status & (1 << 5)) {
        printl("Memory error during FPU lazy state preservation");
    } else if (mem_status & (1 << 4)) {
        printl("Exception stacking caused a memory fault");
    } else if (mem_status & (1 << 3)) {
        printl("Exception unstacking caused a memory fault");
    } else if (mem_status & (1 << 1)) {
        printl("Forbidden load or store address");
    } else if (mem_status & (1 << 0)) {
        printl("Instruction fetch from forbidden address");
    }
}

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

/// The hard fault status register can indicate the reason the the fault. Bit
/// 30 indicates a forced hard fault, generated  by escalating a fault with 
/// configurable priority which can not be served. When this bit is set the
/// hard fault handler must read the status registers of the other fault 
/// exceptions to determine what caused the fault.
void hard_fault(u32* stack_pointer) {
    cpsid_f();
    gpio_clear(GPIOC, 8);

    printl("Hard fault status: %32b", *(volatile u32 *)0xE000ED2C);
    printl("memory fault status: %4h", *(volatile u32 *)0xE000ED34);
    //printl("Usage fault status: %32b", *(volatile u32 *)0xE000ED2A);
    //printl("Bus fault status: %32b", *(volatile u32 *)0xE000ED29);

    //print_usage_fault();
    //print_bus_fault();
    //print_memory_fault();
    

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