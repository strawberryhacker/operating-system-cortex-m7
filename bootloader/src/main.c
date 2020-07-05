/// Copyright (C) StrawberryHacker

#include "types.h"
#include "cpu.h"
#include "nvic.h"
#include "clock.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "watchdog.h"
#include "print.h"
#include "frame.h"
#include "bootloader.h"
#include "memory.h"
#include "flash.h"
#include "cpu.h"
#include "hardware.h"

/// Defines the different commands that might occur in a frame from the host
#define CMD_ERASE_FLASH     0x03
#define CMD_WRITE_PAGE      0x04
#define CMD_WRITE_PAGE_LAST 0x05

/// Defined in `frame.h` and holds the frame information. The user must check 
/// `check_new_frame` before using this
extern volatile struct frame frame;

/// Defines where the next page will programmed into flash; that is the relative
/// offset from the kernel base address at 0x00404000
volatile u32 kernel_page = 0;

/// Temporarily buffer of flash reads and writes
volatile u8 test_flash[512];

int main(void) {

    cpsid_i();

    SYSTICK->CSR = 0x00;

    // Initialize the system
    watchdog_disable();

    gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
    gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
    gpio_clear(GPIOC, 8);

    // Configure the flash wait states
    flash_set_access_cycles(10);

    // Increase the core frequency to 300 MHz and the bus frequency to 150 MHz
    clock_source_enable(CRYSTAL_OSCILLATOR, 0xFF);
    main_clock_select(CRYSTAL_OSCILLATOR);
    plla_init(1, 25, 0b111111);
    master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

    //print_init();
    //frame_init();

    // Check if it is required to stay in the bootloader
    gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
    gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
    gpio_set(GPIOC, 8);
    while (1);
    print("Starting bootloader\n");

    cpsie_f();

    kernel_page = 0;

    // If the bootloader was started from the kernel by the `0x00` packet from 
    // the host, this will notify the host that the CPU has successfully entered
    // the bootloader
    send_response(RESP_OK);

    while (1) {
        if (check_new_frame()) {
            
            if (frame.cmd == CMD_ERASE_FLASH) {
                u8 status = erase_kernel_image((u8 *)frame.payload);
                // Send the status of the operation back to the host
                if (!status) {
                    send_response(RESP_ERROR | RESP_FLASH_ERROR);
                } else {
                    send_response(RESP_OK);
                }

            } else if (frame.cmd == CMD_WRITE_PAGE) {
                printl("Writing page... %d", frame.size);
                // Write a page from offset 0x00404000
                u8 status = write_kernel_page((u8 *)frame.payload, frame.size, 
                    kernel_page++);

                if (status == 0) {
                    send_response(RESP_ERROR | RESP_FLASH_ERROR);
                } else {
                    send_response(RESP_OK);
                }

            } else if (frame.cmd == CMD_WRITE_PAGE_LAST) {
                printl("Writing page... %d", frame.size);
                print("Last page received\n");
                // Write a page from offset 0x00404000
                u8 status = write_kernel_page((u8 *)frame.payload, frame.size, 
                    kernel_page);

                if (status == 0) {
                    send_response(RESP_ERROR | RESP_FLASH_ERROR);
                }

                send_response(RESP_OK);

                print("Starting kernel\n");

                // Firmware download complete
                start_kernel();
            }
        }
    }
}
