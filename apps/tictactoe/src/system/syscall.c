/* Copyright (C) StrawberryHacker */

#include "syscall.h"

void NAKED NOINLINE syscall_thread_sleep(u32 ms) {
    asm volatile ("svc #1");
    asm volatile ("bx lr");
}

void NAKED NOINLINE syscall_gpio_toggle(gpio_reg* port, u8 pin) {
    asm volatile ("svc #2");
    asm volatile ("bx lr");
}

void* NAKED NOINLINE syscall_mm_alloc(u32 size, enum physmem_e region) {
    asm volatile("svc #3");
    asm volatile ("bx lr");
}

void NAKED NOINLINE syscall_mm_free(void* ptr) {
    asm volatile ("svc #4");
    asm volatile ("bx lr");
}

void NAKED NOINLINE syscall_print_byte(u8 data) {
    asm volatile ("svc #5 \n\t");
    asm volatile ("bx lr");
}

u8 NAKED NOINLINE syscall_print_get_status(void) {
    asm volatile ("svc #6 \n\t");
    asm volatile ("bx lr");
}

u32 NAKED NOINLINE syscall_read_print(char* data, u32 size) {
    asm volatile ("svc #7 \n\t");
    asm volatile ("bx lr");
}
