/* Copyright (C) StrawberryHacker */

#include "gpio.h"

void gpio_set_function(gpio_reg* port, u8 pin, enum gpio_func func) {
    /*
     * Check if the pin is a system pin. In this case the syste
     * function must be disabled
     */
    if (port == GPIOB) {
        if ((pin == 4) || (pin == 5) || (pin == 6) || (pin == 7) || (pin == 12)) {
            *((volatile u32 *)(0x40088114)) |= (1 << pin);
        }
    }

    /* If no function is selected the gpio should be controlled by the PIO */
    if (func == GPIO_FUNC_OFF) {
        port->PER = (1 << pin);
    } else {
        /* Select function A, B, C or D */
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

        /* Enable peripheral controll of the pin */
        port->PDR = (1 << pin);
    }
}

void gpio_set_direction(gpio_reg* port, u8 pin, enum gpio_dir dir) {
    if (dir == GPIO_INPUT) {
        port->ODR = (1 << pin);
    } else {
        port->OER = (1 << pin);
    }
}

void gpio_toggle(gpio_reg* port, u8 pin) {
    if (port->ODSR & (1 << pin)) {
        port->CODR = (1 << pin);
    } else {
        port->SODR = (1 << pin);
    }
}

u32 gpio_read(gpio_reg* port) {
    return port->PDSR;
}

u8 gpio_get_pin_status(gpio_reg* port, u8 pin) {
    return (port->PDSR & (1 << pin)) ? 1 : 0;
}

void gpio_set_pull(gpio_reg* port, u8 pin, enum gpio_pull pull) {
    if (pull == GPIO_PULL_DOWN) {
        port->PUDR = (1 << pin);
        port->PPDER = (1 << pin);
    } else {
        port->PUER = (1 << pin);
        port->PPDDR = (1 << pin);
    }
}

void gpio_interrupt_enable(gpio_reg* port, u8 pin, enum gpio_irq_src src)
{
    
    if (src == GPIO_EDGE) {
        port->AIMDR = (1 << pin);
    } else {
        port->AIMER = (1 << pin);

        if (src & 0b10) {
            port->LSR = (1 << pin);     /* Level */
        } else {
            port->ESR = (1 << pin);     /* Edge */
        }

        if (src & 0b01) {
            port->FELLSR = (1 << pin);  /* Falling edge, low level */
        } else {
            port->REHLSR = (1 << pin);  /* Rising edge, high level */
        }
    }

    /* Enabel interrupt */
    port->IER = (1 << pin);
}

u32 gpio_get_interrupt_status(gpio_reg* port)
{
    return port->ISR;
}

void gpio_set_filter(gpio_reg* port, u8 pin, enum gpio_filter filt, u32 us)
{
    /* Select filter */
    if (filt == GPIO_GLITCH_FILTER) {
        port->IFSCDR = (1 << pin);
    } else {
        port->IFSCER = (1 << pin);
    }

    /* The debounce filter period must be programmed */
    if (filt == GPIO_DEBOUNCE_FILTER) {
        u32 div = (u32)((us / 125) - 1.0);
        if (div > 0x3FFF) {
            div = 0x3FFF;
        }
        port->SCDR = (0x3FFF & div);
    }

    /* Enable filter */
    port->IFER = (1 << pin);
}
