#include "debug.h"
#include "gpio.h"
#include "clock.h"
#include "usart.h"
#include "print.h"
#include "nvic.h"

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
}

void debug_deinit(void) {
	peripheral_clock_disable(13);
	usart_deinit(USART0);
	nvic_disable(13);
	nvic_clear_pending(13);
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

void debug_flush(void) {
	usart_flush(USART0);
}