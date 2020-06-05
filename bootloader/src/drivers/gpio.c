#include "gpio.h"

void gpio_set_function(gpio_reg* port, u8 pin, enum gpio_func func) {
    // Check if the is a SYSIO
    if (port == GPIOB) {
        if ((pin == 4) || (pin == 5) || (pin == 6) || (pin == 7) || (pin == 12)) {
            *((volatile u32 *)(0x40088114)) |= (1 << pin);
        }
    }

    if (func == GPIO_FUNC_OFF) {
        port->PER = (1 << pin);
    } else {
        // Select function A, B, C or D
        if (func & 0b1) {
            port->ABCDSR0 |= (1 << pin);
        } else {
            port->ABCDSR0 &= ~(1 << pin);
        }
        if (func & 0b10) {
            port->ABCDSR1 |= (1 << pin);
        } else {
            port->ABCDSR1 &= ~(1 << pin);
        }

        port->PDR = (1 << pin);
    }
}