/* Copyright (C) StrawberryHacker */

#include "bootloader.h"
#include "sections.h"
#include "types.h"
#include "cpu.h"
#include "serial.h"
#include "print.h"
#include "memory.h"
#include "nvic.h"
#include "crc.h"
#include "hardware.h"
#include "clock.h"
#include "cache.h"

#define START_BYTE 0xAA
#define END_BYTE   0x55
#define POLYNOMIAL 0xB2

enum bus_states {
    STATE_IDLE,
    STATE_CMD,
    STATE_SIZE,
    STATE_PAYLOAD,
    STATE_FCS,
    STATE_END
};

/// Frame interface variables
volatile enum bus_states bus_state = STATE_IDLE;
volatile struct frame frame = {0};
volatile u32 frame_index = 0;
volatile u8 frame_received = 0;

struct image_info {
    /* Version numer of the kernel */
    u32 major_version;
    u32 minor_version;

    /* Start address of the bootlaoder */
    u32 bootloader_start;

    /* 
     * The total size allocated to the bootlaoder. This includes the
     * two special structures
     */
    u32 bootloader_size;

    /* Specifies where the bootloader info table is stored */
    u32 bootloader_info;

    /* Start address of the kernel */
    u32 kernel_start;

    /* The allocated size for the kernel */
    u32 kernel_size;

    /* Specifies where the kernel info is stored */
    u32 kernel_info;
};

__bootsig__ u8 boot_signature[32];

__image_info__ struct image_info image_info = {
    .major_version    = 1,
    .minor_version    = 3,

    .bootloader_start = 0x00400000,
    .bootloader_size  = 0x00004000,
    .bootloader_info  = 0x00403E00,

    .kernel_start     = 0x00404000,
    .kernel_size      = 0x001FC000,
    .kernel_info      = 0x00404000
};

/* Stops the clock and stops the timer */
static inline void timeout_stop(void) {
    TIMER0->channel[0].CCR = 0b010;
}

/* Reload the counter by setting it to zero */
static inline void timeout_reload(void) {
    TIMER0->channel[0].CCR = 0b101;
}

void bootloader_init(void) {
    serial_init();

    // Enable the peripheral clock
    peripheral_clock_enable(23);

    // Initialize a programmable clock
    pck_init(PCK6, SLOW_CLOCK, 32);
    pck_enable(PCK6);

    // Set the timer mode
    TIMER0->channel[0].CMR = (1 << 15) | (1 << 7) | (1 << 6);
    TIMER0->channel[0].RC  = 1000;

    // Enable RC compare interrupt
    TIMER0->channel[0].IER = (1 << 4);
    
    // Enable timer 0 channel 0 interrupt
    nvic_enable(23);

    bus_state = STATE_IDLE;

    /* Grab the bootlaoder info section */
    const struct image_info* info = 
        (const struct image_info *)image_info.bootloader_info;

    /* Print the bootloader and kernel version */
    printl("\n\nUsing Vanilla bootloader v%d.%d" ,
        info->major_version, info->minor_version);

    printl("Using Vanilla kernel     v%d.%d\n",
        image_info.major_version, image_info.minor_version);
}

/*
 * Sends a response code back to the host and enable the next frame to be
 * processed
 */
void send_response(u8 error_code) {
    frame_received = 0;
    serial_print("%c", (char)error_code);
}

/* Check if a new frame has been received */
u8 check_new_frame(void) {
    
    if (frame_received) {
        return 1;
    } else {
        return 0;
    }
}

void usart0_exception(void) {
	char data = serial_read();

    // If the bus state is not IDLE the timeout interface will be reloaded
    if (bus_state != STATE_IDLE) {
        timeout_reload();
    }

	if ((data == 0x00) && (bus_state == STATE_IDLE)) {

        /* The bootloader does not use cache and is self modifying */
        //dcache_disable();
	    //icache_disable();

		print_flush();
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		
		/* Perform a soft reset */
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}

    // Frame state machine FSM
    switch (bus_state) {
        case STATE_IDLE : {
            if (data == START_BYTE) {
                bus_state = STATE_CMD;

                // Take care of the edge case where only one byte is written
                timeout_reload();
            }
            break;
        }
        case STATE_CMD : {
            frame.cmd = data;
            bus_state = STATE_SIZE;
            frame_index = 0;
            frame.size  = 0;
            break;
        }
        case STATE_SIZE : {
            // Receive 2 bytes in little endian
            frame.size |= (data << (8 * frame_index++));
            
            if (frame_index >= 2) {
                bus_state = STATE_PAYLOAD;
                frame_index = 0;
            }
            break;
        }
        case STATE_PAYLOAD : {
            frame.payload[frame_index++] = data;

            if (frame_index >= frame.size) {
                bus_state = STATE_FCS;
            }
            break;
        }
        case STATE_FCS : {
            frame.fcs = data;
            bus_state = STATE_END;
            break;
        }
        case STATE_END : {
            if (data == END_BYTE) {
                // Check the FCS of the frame
                u8 fcs = crc_calculate((u8 *)&frame, frame.size + 3, POLYNOMIAL);

                if (fcs == frame.fcs) {
                    frame_received = 1;
                } else {
                    send_response(RESP_ERROR | RESP_FCS_ERROR);
                }
            } else {
                printl("Frame error");
                send_response(RESP_ERROR | RESP_FRAME_ERROR);
            }
            bus_state = STATE_IDLE;
            timeout_stop();
            break;
        }
    }
}

/* Timeout handler */
void timer0_ch0_handler(void) {
    // Clear timer flags
    (void)TIMER0->channel[0].SR;

    printl("Kernel timeout");

    timeout_stop();
    bus_state = STATE_IDLE;
}
