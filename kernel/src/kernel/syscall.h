#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "hardware.h"
#include "mm.h"
#include "cpu.h"

#define NAKED __attribute__((naked))

void NOINLINE syscall_thread_sleep(u32 ms) NAKED;

void NOINLINE syscall_gpio_toggle(gpio_reg* port, u8 pin) NAKED;

void* NOINLINE syscall_mm_alloc(u32 size, enum physmem_e region) NAKED;

void NOINLINE syscall_mm_free(void* ptr) NAKED;

#endif