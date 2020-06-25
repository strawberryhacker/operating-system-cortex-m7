#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "hardware.h"

enum physmem_e {
    SRAM,
    DRAM_BANK_1,
    DRAM_BANK_2_1k,
    DRAM_BANK_2_4k
};

#define NAKED __attribute__((naked))

void syscall_thread_sleep(u32 ms) NAKED;

void syscall_gpio_toggle(gpio_reg* port, u8 pin) NAKED;

void* syscall_mm_alloc(u32 size, enum physmem_e region) NAKED;

void syscall_mm_free(void* ptr) NAKED;

#endif