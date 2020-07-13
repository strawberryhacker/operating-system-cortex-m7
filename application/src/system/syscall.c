/// Copyright (C) StrawberryHacker

#include "syscall.h"

void NAKED INLINE syscall_thread_sleep(u32 ms) {
    asm volatile ("svc #1");
    asm volatile ("bx lr");
}

void NAKED INLINE syscall_gpio_toggle(gpio_reg* port, u8 pin) {
    asm volatile ("svc #2");
    asm volatile ("bx lr");
}

void* NAKED INLINE syscall_mm_alloc(u32 size, enum physmem_e region) {
    asm volatile("svc #3");
    asm volatile ("bx lr");
}

void NAKED INLINE syscall_mm_free(void* ptr) {
    asm volatile ("svc #4");
    asm volatile ("bx lr");
}