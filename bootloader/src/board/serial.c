/// Copyright (C) StrawberryHacker

#include "serial.h"
#include "usart.h"
#include "clock.h"
#include "gpio.h"
#include "sprint.h"
#include "nvic.h"

#include <stdarg.h>

/// The `sprint` formatter functions will output the result to this buffer. This
/// buffer sets the maximum print limit supported by the system
static char serial_buffer[256];

/// Initializes the system serial port USART0 with the following configuration
///
/// Baud rate - 115200
/// Stop bit  - one
/// Parity    - disabled
/// Interrupt - receive complete
void serial_init(void) {

    // Enable the serial module to control the pins
    gpio_set_function(GPIOB, 0, GPIO_FUNC_C);
    gpio_set_function(GPIOB, 1, GPIO_FUNC_C);

    // Enable the peripheral clock of the serial module
    peripheral_clock_enable(13);

    // Configure the serial module
    struct usart_desc serial = {
        .data_bits = USART_DATA_8_BIT,
        .parity    = USART_PARITY_NO,
        .stop_bits = USART_SB_ONE,
        .buad_rate = 230400
    };
    usart_init(USART0, &serial);

    // Enable the serial peripheral to generate receive complete interrupt
    usart_interrupt_enable(USART0, USART_IRQ_RXRDY);

    // Enable the NVIC
    nvic_enable(13);
}

/// Deinitializes the serial port USART0 for soft reset support
void serial_deinit(void) {
	usart_deinit(USART0);
	peripheral_clock_disable(13);
	nvic_disable(13);
	nvic_clear_pending(13);
}

/// Serial port USART0 formatted printing
void serial_print(const char* data, ...) {
    va_list obj;

    // Pass forward the VA object containing the optional arguments. The second
    // parameter in `va_start` must be the argument that precedes the (...)
    va_start(obj, data);
    u32 size = print_to_buffer_va(serial_buffer, data, obj);
    va_end(obj);

    // Transmit the formated buffer
    const char* src = serial_buffer;
    while (size--) {
        usart_write(USART0, *src++);
    }
}

void serial_printl(const char* data, ...) {
    va_list obj;

    // Pass forward the VA object containing the optional arguments. The second
    // parameter in `va_start` must be the argument that precedes the (...)
    va_start(obj, data);
    u32 size = print_to_buffer_va(serial_buffer, data, obj);
    va_end(obj);

    // Transmit the formated buffer
    const char* src = serial_buffer;
    while (size--) {
        usart_write(USART0, *src++);
    }
    usart_write(USART0, '\n');
}

/// Flushes the USART1 transmit buffer. The `serial_print` only checks the 
/// transmitter status before the THR write, not after. If the user depends on 
/// the characters beeing transmitted before proceeding this function can be
/// called
void serial_flush(void) {
    usart_flush(USART0);
}

/// Reads the RHR register. This does not check if the new data is valid
u8 serial_read(void) {
    return usart_read(USART0);
}
