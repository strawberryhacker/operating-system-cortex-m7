/* Copyright (C) StrawberryHacker */

#include "print.h"
#include "gpio.h"
#include "clock.h"
#include "usart.h"
#include "sprint.h"
#include "nvic.h"
#include "ringbuffer.h"

#include <stdarg.h>

/*
 * The `sprint` formatter functions will output the result to this
 * buffer. This buffer sets the maximum print limit supported by the
 * system
 */
static char debug_buffer[64];
static char receive_buffer[256];

struct ringbuffer rb;

/*
 * Initializes the system serial port USART1 with the following configuration
 *
 * Baud rate - 115200
 * Stop bit  - one
 * Parity    - disabled
 * Interrupt - disabled
 */
void print_init(void) {

    /* Enable the serial module to control the pins */
    gpio_set_function(GPIOA, 21, GPIO_FUNC_A);
    gpio_set_function(GPIOB, 4, GPIO_FUNC_D);

    /* Enable the peripheral clock of the serial module */
    peripheral_clock_enable(14);

    /* Configure the serial module */
    struct usart_desc debug_usart = {
        .data_bits = USART_DATA_8_BIT,
        .parity    = USART_PARITY_NO,
        .stop_bits = USART_SB_ONE,
        .buad_rate = 115200
    };
    usart_init(USART1, &debug_usart);

    /* Enable receive complete interrupt */
    usart_interrupt_enable(USART1, USART_IRQ_RXRDY);

    /* Set up the ringbuffer */
    ringbuffer_init(&rb, (u8 *)receive_buffer, 256);

    /* Set up the NVIC */
    nvic_enable(14);
    nvic_set_prioriy(14, NVIC_PRI_2);
}

/*
 * Deinitializes the serial port USART1 for soft reset support
 */
void print_deinit(void) {
	usart_deinit(USART1);
    peripheral_clock_disable(14);
	nvic_disable(14);
	nvic_clear_pending(14);
}

/*
 * Serial port USART1 formatted printing
 */
void print(const char* data, ...) {
    va_list obj;

    /*
     * Pass forward the VA object containing the optional arguments.
     * The second parameter in `va_start` must be the argument that
     * precedes the (...)
     */
    va_start(obj, data);
    u32 size = print_to_buffer_va(debug_buffer, data, obj);
    va_end(obj);

    /* Transmit the formated buffer */
    const char* src = debug_buffer;
    while (size--) {
        usart_write(USART1, *src++);
    }
}

/*
 * Serial port USART1 formatted printing with an automatic new line
 */
void printl(const char* data, ...) {
    va_list obj;

    /*
     * Pass forward the VA object containing the optional arguments.
     * The second parameter in `va_start` must be the argument that
     * precedes the (...)
     */
    va_start(obj, data);
    u32 size = print_to_buffer_va(debug_buffer, data, obj);
    va_end(obj);

    /* Transmit the formated buffer */
    const char* src = debug_buffer;
    while (size--) {
        usart_write(USART1, *src++);
    }
    usart_write(USART1, '\n');
}

/*
 * Prints a raw string without any formatting
 */
void print_raw(const char* data) {
    while (*data) {
        usart_write(USART1, *data++);
    }
}

/*
 * Prints `size` number of bytes from memory in a nice format
 */
void print_memory(const u32* memory, u32 size) {
    const u8* src = (const u8 *)memory;
    u8 line = 0;
    u8 byte = 0;
    u8 section = 0;
    print("\n0x%4h:  ", (u32)src);
    for (u32 i = 0; i < size; i++) {
        if (line >= 2) {
            line = 0;
            if (section++ >= 8) {
                section = 0;
                print("\n");
            }
            print("\n0x%4h:  ", (u32)src);
        }
        print("%1h ", *src++);

        if (byte++ >= 3) {
            byte = 0;
            line++;
            print("   ");
        }
    }
    print("\n\n");
}

/*
 * Flushes the USART1 transmit buffer. The `serial_print` only
 * checks the transmitter status before the THR write, not after.
 * If the user depends on the characters beeing transmitted before
 * proceeding this function can be called
 */
void print_flush(void) {
	usart_flush(USART1);
}

/*
 * Print `count` number of characters from the specified string
 */
void print_count(const char* data, u32 count) {
    while (count--) {
        usart_write(USART1, *data++);
    }
}

/*
 * Print one byte to the console whithout checking the buffer status.
 * The user must check for the print status before calling this
 */
void print_byte(u8 data) {
    usart_write_raw(USART1, data);
}

/*
 * Check the THR status. Returns 1 if the transmit buffer is ready for
 * new data
 */
u8 print_get_status(void) {
    return usart_get_thr_status(USART1);
}

/*  
 * Try to read a number of bytes from the receive buffer into
 * the data pointer and returns the number of bytes read.
 */
u32 read_print_buffer(char* data, u32 size) {
    u32 status = ringbuffer_read_mult(&rb, (u8 *)data, size);

    return status;
}

/*
 * Interrupt exeption is called when a new character is
 * available in the reive holding register
 */
void usart1_exception(void) {

    /* Read the RHR to clear the interrupt flag */
    u8 rec = usart_read(USART1);

    ringbuffer_add(&rb, rec);
}
