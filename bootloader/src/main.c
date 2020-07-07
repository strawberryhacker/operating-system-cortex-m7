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
#include "hash.h"
#include "panic.h"

/// Defines the different commands that might occur in a frame from the host
#define CMD_ERASE_FLASH      0x03
#define CMD_WRITE_PAGE       0x04
#define CMD_WRITE_PAGE_LAST  0x05
#define CMD_SET_FLASH_OFFSET 0x06  // Offset from 0x00404000
#define CMD_CTRL_LED         0x07

/// Defined in `frame.h` and holds the frame information. The user must check 
/// `check_new_frame` before using this
extern volatile struct frame frame;

/// Defines where the next page will programmed into flash; that is the relative
/// offset from the kernel base address at 0x00404000. This can be modified by
/// using CMD 0x06
extern volatile u32 kernel_page;

int main(void) {

    // Initialize the system
    watchdog_disable();

    // Configure the flash wait states
    flash_set_access_cycles(7);

    // Increase the core frequency to 300 MHz and the bus frequency to 150 MHz
    clock_source_enable(CRYSTAL_OSCILLATOR, 0xFF);
    main_clock_select(CRYSTAL_OSCILLATOR);
    plla_init(1, 25, 0b111111);
    master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

    print_init();

    // Initialize the on-board LED
    gpio_set_function(GPIOC, 8, GPIO_FUNC_OFF);
    gpio_set_direction(GPIOC, 8, GPIO_OUTPUT);
    gpio_set(GPIOC, 8);

    // Initializing the on-board button which will be used as a boot trigger
    peripheral_clock_enable(10);
	gpio_set_function(GPIOA, 11, GPIO_FUNC_OFF);
	gpio_set_direction(GPIOA, 11, GPIO_INPUT);
	gpio_set_pull(GPIOA, 11, GPIO_PULL_UP);

    // Enable the hash engine peripheral clock
    peripheral_clock_enable(32);

    // The normal case is a gereral reboot or POR and the kernel should be 
    // loaded. However, in case of a firmware upgrade or a kernel crash the 
    // cpu might skip the loading stage and go straight to the bootloader. 
    // Assume normal kernel loading
    u8 execute_kernel = 1;

    // Checks for the "StayInBootloader" signature in RAM boot signature area
    if (check_boot_signature() == 1) {
        execute_kernel = 0;
        clear_boot_signature();
    }
    // Check if the boot pin is being pressed
    if (gpio_get_pin_status(GPIOA, 11) == 0) {
        execute_kernel = 0;
    }

    // If no triggers present try to load the kernel
    if (execute_kernel) {
        u8 kernel_ok = 1;

        // Check if the info matches
        if (check_info_match() == 0) {
            kernel_ok = 0;
        }
        // Check the hash value
        if (verify_kernel_hash() == 0) {
            kernel_ok = 0;
        }
        
        // If the kernel is ok we can try to load it
        if (kernel_ok) {
            start_kernel();
        }
    }

    // Initialize the frame interfaces so that incoming frames can be processed
    frame_init();

    cpsie_f();

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
                //printl("Writing page... %d", frame.size);
                // Write a page from offset 0x00404000
                u8 status = write_kernel_page((u8 *)frame.payload, frame.size, 
                    kernel_page++);

                if (status == 0) {
                    send_response(RESP_ERROR | RESP_FLASH_ERROR);
                } else {
                    send_response(RESP_OK);
                }

            } else if (frame.cmd == CMD_WRITE_PAGE_LAST) {
                //printl("Writing page... %d", frame.size);
                //print("Last page received\n");
                // Write a page from offset 0x00404000
                u8 status = write_kernel_page((u8 *)frame.payload, frame.size, 
                    kernel_page);

                if (status == 0) { 
                    send_response(RESP_ERROR | RESP_FLASH_ERROR);
                }
                send_response(RESP_OK);

                if (store_kernel_hash() == 0) {
                    panic("Cannot store the hash digest");
                }

                // Firmware download complete. A chip reset will never stay in
                // the bootloader unless either the hash value is wrong (should
                // never happend) or is the infor structures don't match
                print_flush();
                cpsid_i();
		        *((u32 *)0x400E1800) = 0xA5000000 | 0b1;

            } else if (frame.cmd == CMD_SET_FLASH_OFFSET) {
                set_flash_write_offset((u8 *)frame.payload);
                send_response(RESP_OK);

            } else if (frame.cmd == CMD_CTRL_LED) {
                if (frame.payload[0] == 0) {
                    gpio_set(GPIOC, 8);
                } else {
                    gpio_clear(GPIOC, 8);
                }
                send_response(RESP_OK);
            }
        }
    }
}
