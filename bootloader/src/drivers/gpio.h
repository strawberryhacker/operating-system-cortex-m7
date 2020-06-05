#ifndef GPIO_H
#define GPIO_H

#include "types.h"
#include "hardware.h"

enum gpio_func {
    GPIO_FUNC_A,
    GPIO_FUNC_B,
    GPIO_FUNC_C,
    GPIO_FUNC_D,
    GPIO_FUNC_OFF
};

void gpio_set_function(gpio_reg* port, u8 pin, enum gpio_func func);

#endif