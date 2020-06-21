/// Copyright (C) StrawberryHacker

#include "print.h"
#include "gpio.h"
#include "clock.h"
#include "usart.h"
#include "sprint.h"
#include "nvic.h"

#include <stdarg.h>

/// The debug buffer is used by the sprint function to format the output. 
/// Therefore the maximum print length is `64` bytes.
static char debug_buffer[64];

/// Initializes the serial port 0 of the SBC to 115200, one stop bit, no parity
/// and enables receive complete interrupt
void print_init(void) {

    // Map the right function to the pins
    gpio_set_function(GPIOB, 0, GPIO_FUNC_C);
    gpio_set_function(GPIOB, 1, GPIO_FUNC_C);

    peripheral_clock_enable(13);

    struct usart_desc debug_usart;
    debug_usart.data_bits = USART_DATA_8_BIT;
    debug_usart.parity = USART_PARITY_NO;
    debug_usart.stop_bits = USART_SB_ONE;
    debug_usart.buad_rate = 115200;

    // This will enable the serial interface
    usart_init(USART0, &debug_usart);

    // Enable interrupt in both peripheral registers and in the NVIC
    usart_interrupt_enable(USART0, USART_IRQ_RXRDY);
    nvic_enable(13);
}

/// Releases the serial resources used
void print_deinit(void) {
	peripheral_clock_disable(13);
	usart_deinit(USART0);
	nvic_disable(13);
	nvic_clear_pending(13);
}

void print(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    // Pass forward the VA object containing the optional arguments
    u32 size = print_to_buffer_va(debug_buffer, data, obj);

    // Transmit the formated buffer on serial port 0
    char* src = debug_buffer;
    while (size--) {
        usart_write(USART0, *src++);
    }
}

void printl(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    // Pass forward the VA object containing the optional arguments
    u32 size = print_to_buffer_va(debug_buffer, data, obj);

    // Transmit the formated buffer on serial port 0
    char* src = debug_buffer;
    while (size--) {
        usart_write(USART0, *src++);
    }
    
    // Print a new line
    usart_write(USART0, '\n');
}

/// The `serial_print` function only checks is the USART transmit buffer is 
/// ready, and does not block to the character is transmitted. If all characters
/// needs to be transmitted before proceeding, this function can be called
void print_flush(void) {
	usart_flush(USART0);
}
