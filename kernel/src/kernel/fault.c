/* Copyright (C) StrawberryHacker */

#include "panic.h"
#include "gpio.h"
#include "print.h"
#include "cpu.h"

void exception_fault(void) {
    panic("Exception not handled - default handler triggered");
}

static inline __attribute__((always_inline)) void print_bus_fault(void) {
    volatile u32 bus_status = ((*(volatile u32 *)0xE000ED28) >> 8) & 0xFF;

    if (bus_status & 0xFF) {
        printl("Bus fault has occured: ");
    }

    if (bus_status & (1 << 7)) {
        printl("Bus fault at address 0x%4h", SCB->BFAR);
    }
    if (bus_status & (1 << 5)) {
        printl("Bus fault during FPU lazy state preservation");
    }
    if (bus_status & (1 << 4)) {
        printl("Exception stacking caused a bus fault");
    } 
    if (bus_status & (1 << 3)) {
        printl("Exception unstacking caused a bus fault");
    } 
    if (bus_status & (1 << 2)) {
        printl("Imprecise bus fault");
    } 
    if (bus_status & (1 << 1)) {
        printl("Precise error - PC contains the reason");
    } 
    if (bus_status & (1 << 0)) {
        printl("Instruction bus error");
    }
}

static inline __attribute__((always_inline)) void print_usage_fault(void) {
    volatile u32 usage_status = ((*(volatile u32 *)0xE000ED28) >> 16) && 0xFFFF;

    if (usage_status & 0xFFFF) {
        printl("Usage fault has occured: ");
    }

    if (usage_status & (1 << 9)) {
        printl("Devision by zero");
    } 
    if (usage_status & (1 << 8)) {
        printl("Unaligned memory access");
    } 
    if (usage_status & (1 << 3)) {
        printl("Coprocessor error - instruction not supported");
    } 
    if (usage_status & (1 << 2)) {
        printl("Illegal use of EXE_RETURN");
    } 
    if (usage_status & (1 << 1)) {
        printl("Illegal use of ePSR");
    } 
    if (usage_status & (1 << 0)) {
        printl("Undefined instruction");
    }
}

static inline __attribute__((always_inline)) void print_memory_fault(void) {
    volatile u32 mem_status = (*(volatile u32 *)0xE000ED28) & 0xFF;

    if (mem_status & 0xFF) {
        printl("Memory fault has occured:");
    }

    if (mem_status & (1 << 7)) {
        printl("MMAR hold the fault address");
    } 
    if (mem_status & (1 << 5)) {
        printl("Memory error during FPU lazy state preservation");
    } 
    if (mem_status & (1 << 4)) {
        printl("Exception stacking caused a memory fault");
    } 
    if (mem_status & (1 << 3)) {
        printl("Exception unstacking caused a memory fault");
    } 
    if (mem_status & (1 << 1)) {
        printl("Forbidden load or store address");
    } 
    if (mem_status & (1 << 0)) {
        printl("Instruction fetch from forbidden address");
    }
}

/*
 * Print the reason for the hard fault to the console
 */
void print_hard_fault(void) {
    u32 hard_fault_status = SCB->HFSR;

    if (hard_fault_status & (1 << 30)) {
        printl("Forced hard fault escalated from a none servicable fault handler");
    }
    /* This fault is allways handled by the hard fault execption */
    if (hard_fault_status & (1 << 1)) {
        printl("Bus fault during vector table read");
    }
}

void mem_fault(void) {
    cpsid_f();
    printl("Memory\n");
    panic("Memory fault");
}

void bus_fault(void) {
    cpsid_f();
    printl("Bus\n");
    panic("Bus fault");
}

void usage_fault(void) {
    cpsid_f();
    printl("Usage\n");
    panic("Usage fault");
}

/*
 * The hard fault status register can indicate the reason the the
 * fault. Bit 30 indicates a forced hard fault, generated  by
 * escalating a fault with configurable priority which can not be
 * served. When this bit is set the hard fault handler must read
 * the status registers of the other fault exceptions to determine
 * what caused the fault.
 */
void hard_fault(u32* stack_pointer) {
    cpsid_f();

    print(ANSI_RED "Hard fault occured:\n");
    print_hard_fault();
    print("\n" ANSI_NORMAL);

    print_bus_fault();
    print_memory_fault();
    print_usage_fault();

    printl("\nStack pointer: 0x%4h", stack_pointer);
    printl("PC: 0x%4h\n", stack_pointer[6]);
    printl(BLUE "R0:  0x%4h", cpu_get_r0());
    printl("R1:  0x%4h", cpu_get_r1());
    printl("R2:  0x%4h", cpu_get_r2());
    printl("R3:  0x%4h", cpu_get_r3());
    printl("R4:  0x%4h", cpu_get_r4());
    printl("R5:  0x%4h", cpu_get_r5());
    printl("R6:  0x%4h", cpu_get_r6());
    printl("R7:  0x%4h", cpu_get_r7());
    printl("R8:  0x%4h", cpu_get_r8());
    printl("R9:  0x%4h", cpu_get_r9());
    printl("R10: 0x%4h", cpu_get_r10());
    printl("R11: 0x%4h", cpu_get_r11());
    printl("R12: 0x%4h", cpu_get_r12());
    printl("R13: 0x%4h", cpu_get_r13());
    printl("LR:  0x%4h", cpu_get_lr());
    printl("PC:  0x%4h\n" ANSI_NORMAL, cpu_get_pc());

    /* We want to stop execution here */
    panic("Hard fault");
}
