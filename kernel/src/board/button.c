/* Copyright (C) StrawberryHacker */

#include "button.h"
#include "gpio.h"
#include "hardware.h"
#include "print.h"
#include "clock.h"
#include "nvic.h"

static struct list_node button_list;

void button_init(void)
{
    /* Configure the on board button */
	peripheral_clock_enable(10);	
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);
	gpio_interrupt_enable(GPIOA, 11, GPIO_EDGE);
	gpio_set_filter(GPIOA, 11, GPIO_DEBOUNCE_FILTER, 10000);
	nvic_enable(10);

    list_init(&button_list);
}

void button_add_handler(struct button_callback* handler)
{
    list_add_first(&handler->node, &button_list);
}

void gpioa_exception(void)
{
	gpio_get_interrupt_status(GPIOA);

    /* Edge detection is on */
    u8 pin_level = (GPIOA->PDSR & (1 << 11)) ? 1 : 0;
	
    struct list_node* node;
    list_iterate(node, &button_list) {

        struct button_callback* cb =
            list_get_entry(node, struct button_callback, node);

        if ((cb->event == BUTTON_PRESSED) && (pin_level == 1)) {
            continue;
        }
        if ((cb->event == BUTTON_RELEASED) && (pin_level == 0)) {
            continue;    
        }
        cb->callback();
    }
}
