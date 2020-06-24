#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "hardware.h"

void syscall_thread_sleep(u32 ms);

void syscall_gpio_toggle(gpio_reg* port, u8 pin);

#endif