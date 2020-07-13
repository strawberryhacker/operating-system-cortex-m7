/// Copyright (C) StrawberryHacker

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "hardware.h"

enum physmem_e {
    SRAM,
    DRAM_BANK_1
};

#define NAKED __attribute__((naked))
#define NOINLINE __attribute__((noinline))

void NAKED NOINLINE syscall_thread_sleep(u32 ms);

void NAKED NOINLINE syscall_gpio_toggle(gpio_reg* port, u8 pin);

void* NAKED NOINLINE syscall_mm_alloc(u32 size, enum physmem_e region);

void NAKED NOINLINE syscall_mm_free(void* ptr);

#endif