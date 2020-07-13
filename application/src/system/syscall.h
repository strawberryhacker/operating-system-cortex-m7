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
#define INLINE __attribute__((allways_inline))

void NAKED INLINE syscall_thread_sleep(u32 ms);

void NAKED INLINE syscall_gpio_toggle(gpio_reg* port, u8 pin);

void* NAKED INLINE syscall_mm_alloc(u32 size, enum physmem_e region);

void NAKED INLINE syscall_mm_free(void* ptr);

#endif