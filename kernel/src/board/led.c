/* Copyright (C) StrawberryHacker */

#include "led.h"
#include "hardware.h"
#include "gpio.h"

void led_init(void)
{
    /* Configure the on-board LED */
	gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
}

void led_on(void)
{
    gpio_clear(GPIOC, 8);
}

void led_off(void)
{
    gpio_set(GPIOC, 8);
}

void led_toggle(void)
{
    gpio_toggle(GPIOC, 8);
}
