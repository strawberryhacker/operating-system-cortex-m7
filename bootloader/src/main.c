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

extern volatile struct frame frame;

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

    print("Starting\n");

    cpsie_f();


    while (1) {
        if (check_new_frame()) {
            // Do something

            printl("CMD: 0x%1h", frame.cmd);
            printl("Size: %d", frame.size);
           
            for (u32 i = 0; i < frame.size; i++) {
                print("%c", frame.payload[i]);
            }
            print("\n");
            printl("FCS: %d", frame.fcs);
            print("\n");

            send_response(RESP_OK);
        }
    }
}
