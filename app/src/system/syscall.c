#include "syscall.h"

void syscall_thread_sleep(u32 ms) {
    asm volatile ("svc #1");
}

void syscall_gpio_toggle(gpio_reg* port, u8 pin) {
    asm volatile ("svc #2");
}