#include "debug.h"
#include "gpio.h"
#include "clock.h"
#include "usart.h"
#include "print.h"
#include "interrupt.h"

#include <stdarg.h>

void debug_init(void) {
    gpio_set_function(GPIOB, 0, GPIO_FUNC_C);
    gpio_set_function(GPIOB, 1, GPIO_FUNC_C);

    peripheral_clock_enable(13);

    struct usart_desc debug_usart;
    
    debug_usart.data_bits = USART_DATA_8_BIT;
    debug_usart.parity = USART_PARITY_NO;
    debug_usart.stop_bits = USART_SB_ONE;
    debug_usart.buad_rate = 115200;

    usart_init(USART0, &debug_usart);

    usart_interrupt_enable(USART0, USART_IRQ_RXRDY);
    interrupt_enable(13);
}

static char debug_buffer[64];

void debug_print(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    u32 size = print_to_buffer_va(debug_buffer, data, obj);
    char* src = debug_buffer;
    while (size--) {
        usart_write(USART0, *src++);
    }
}