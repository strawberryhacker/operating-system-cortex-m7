/// Copyright (C) StrawberryHacker

#include "panic.h"
#include "gpio.h"

void mem_fault_handler(void) {
    gpio_clear(GPIOC, 8);
    panic("Memory fault");
}

void bus_fault_handler(void) {
    gpio_clear(GPIOC, 8);
    panic("Bus fault");
}

void usage_fault_handler(void) {
    gpio_clear(GPIOC, 8);
    panic("Usage fault");
}

void hard_fault_handler(void) {
    gpio_clear(GPIOC, 8);
    panic("Hard fault");
}