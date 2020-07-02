/// Copyright (C) StrawberryHacker

#include "serial.h"
#include "usart.h"
#include "clock.h"
#include "gpio.h"
#include "sprint.h"
#include "nvic.h"

#include <stdarg.h>

/// The debug buffer is used by the sprint function to format the output. 
/// Therefore the maximum print length is `64` bytes.
static char serial_buffer[256];

/// Initializes the serial port 1 of the SBC to 115200, one stop bit, no parity
/// and enables receive complete interrupt
void serial_init(void) {
    // Enable the peripheral clock
    peripheral_clock_enable(13);

    // Trigger en interrupt on data received
    usart_interrupt_enable(USART0, USART_IRQ_RXRDY);

    // Enable GPIOs
    gpio_set_function(GPIOB, 0, GPIO_FUNC_C);
    gpio_set_function(GPIOB, 1, GPIO_FUNC_C);

    // Configure the peripheral
    struct usart_desc serial;
    serial.buad_rate = 19200;
    serial.data_bits = USART_DATA_8_BIT;
    serial.parity = USART_PARITY_NO;
    serial.stop_bits = USART_SB_ONE;

    usart_init(USART0, &serial);

    nvic_enable(13);
}

/// Releases the serial resources used
void serial_deinit(void) {
	peripheral_clock_disable(13);
	usart_deinit(USART0);
	nvic_disable(13);
	nvic_clear_pending(13);
}

void serial_print(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    // Pass forward the VA object containing the optional arguments
    u32 size = print_to_buffer_va(serial_buffer, data, obj);
    va_end(obj);

    // Transmit the formated buffer on serial port 0
    const char* buffer_ptr = serial_buffer;
    while (size--) {
        usart_write(USART0, *buffer_ptr++);
    }
}

void serial_printl(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    // Pass forward the VA object containing the optional arguments
    u32 size = print_to_buffer_va(serial_buffer, data, obj);
    va_end(obj);

    // Transmit the formated buffer on serial port 0
    const char* buffer_ptr = serial_buffer;
    while (size--) {
        usart_write(USART0, *buffer_ptr++);
    }
    

    usart_write(USART0, '\n');
}

u8 serial_read(void) {
    return usart_read(USART0);
}

void serial_flush(void) {
    usart_flush(USART0);
}