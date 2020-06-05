#include "serial.h"
#include "usart.h"
#include "clock.h"
#include "gpio.h"
#include "print.h"
#include "interrupt.h"
#include <stdarg.h>

static char serial_buffer[256];

void serial_init(void) {
    // Enable the peripheral clock
    peripheral_clock_enable(14);

    // Trigger en interrupt on data received
    usart_interrupt_enable(USART1, USART_IRQ_RXRDY);

    // Enable GPIOs
    gpio_set_function(GPIOA, 21, GPIO_FUNC_A);
    gpio_set_function(GPIOB, 4, GPIO_FUNC_D);

    // Configure the peripheral
    struct usart_desc serial;
    serial.buad_rate = 115200;
    serial.data_bits = USART_DATA_8_BIT;
    serial.parity = USART_PARITY_NO;
    serial.stop_bits = USART_SB_ONE;

    usart_init(USART1, &serial);

    interrupt_enable(14);
}

void serial_write(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    u32 size = print_to_buffer_va(serial_buffer, data, obj);

    const char* buffer_ptr = serial_buffer;
    while (size--) {
        usart_write(USART1, *buffer_ptr++);
    }

    va_end(obj);
}

u8 serial_read(void) {
    return usart_read(USART1);
}