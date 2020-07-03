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

#define CMD_ERASE_FLASH     0x03
#define CMD_WRITE_PAGE      0x04
#define CMD_WRITE_PAGE_LAST 0x05

extern volatile struct frame frame;
volatile u32 kernel_page = 0;


volatile u8 test_flash[512];

void print_frame(void) {
    printl("XXX: 0x%1h", frame.cmd);
    printl("Size: %d", frame.size);

    for (u32 i = 0; i < frame.size; i++) {
        print("%c", frame.payload[i]);
    }
    print("\n\n");
}

int main(void) {

    // Initialize the system
    watchdog_disable();

    // Configure the flash wait states
    flash_set_access_cycles(7);

    // Increase the core frequency to 300 MHz and the bus frequency to 150 MHz
    clock_source_enable(CRYSTAL_OSCILLATOR);
    main_clock_select(CRYSTAL_OSCILLATOR);
    plla_init(1, 25, 0b111111);
    master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);

    print_init();
    frame_init();

    // Check if it is required to stay in the bootloader

    print("Starting bootloader\n");

    cpsie_f();

    kernel_page = 0;

    while (1) {
        if (check_new_frame()) {
            
            if (frame.cmd == CMD_ERASE_FLASH) {
                print_frame();
                //u8 status = erase_kernel_image((u8 *)frame.payload);
                u8 status = flash_erase_image(20000);
                print("Erased kernel image\n");
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
