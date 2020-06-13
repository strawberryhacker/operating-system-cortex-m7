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

    nvic_enable(14);
}

/// Releases the serial resources used
void serial_deinit(void) {
	peripheral_clock_disable(14);
	usart_deinit(USART1);
	nvic_disable(14);
	nvic_clear_pending(14);
}

void serial_print(const char* data, ...) {
    va_list obj;
    va_start(obj, data);

    // Pass forward the VA object containing the optional arguments
    u32 size = print_to_buffer_va(serial_buffer, data, obj);

    // Transmit the formated buffer on serial port 0
    const char* buffer_ptr = serial_buffer;
    while (size--) {
        usart_write(USART1, *buffer_ptr++);
    }

    va_end(obj);
}

u8 serial_read(void) {
    return usart_read(USART1);
}