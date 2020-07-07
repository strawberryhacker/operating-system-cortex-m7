#include "frame.h"
#include "serial.h"
#include "print.h"
#include "hardware.h"
#include "clock.h"
#include "nvic.h"
#include "crc.h"

#define START_BYTE 0xAA
#define END_BYTE   0x55
#define POLYNOMIAL 0xB2

/// List over the different states in the receive handler
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
volatile u8 frame_received;

/// Stops the clock and stops the timer
static inline void timeout_stop(void) {
    TIMER0->channel[0].CCR = 0b010;
}

/// Reload the counter by setting it to zero
static inline void timeout_reload(void) {
    TIMER0->channel[0].CCR = 0b101;
}

/// Initializes all peripheral needed to start receiving frames. This includes
/// a serial port, a general purpose hardware timer and a programmable clock
/// PCK6 which will provide a slow enough clock for the timeout
void frame_init(void) {

    // This configures the onboard CDC communication with the host
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
}

/// Releases all resources used in the frame process interface
void frame_deinit(void) {
    serial_deinit();
    peripheral_clock_disable(23);
    pck_disable(PCK6);
    timeout_stop();
    TIMER0->channel[0].IDR = 0xFFFFFFFF;
    nvic_disable(23);
    nvic_clear_pending(23);
}

/// Sends a response code back to the host and enable the next frame to be
/// processed
void send_response(u8 error_code) {
    frame_received = 0;
    serial_print("%c", (char)error_code);
}

/// Returns `1` if a new frame has been successfully received and `0` if not
u8 check_new_frame(void) {
    
    if (frame_received) {
        return 1;
    } else {
        return 0;
    }
}

/// Frame receive handler that process all incoming frames
void usart0_handler(void) {
    char data = serial_read();

    // If the bus state is not IDLE the timeout interface will be reloaded
    if (bus_state != STATE_IDLE) {
        timeout_reload();
    }

    // The host sends a `0x00` to the target to instruct it to go to the
    // bootloader. When the target enters bootloader is will send a `RESP_OK`
    // back to the host. If the bootloader is already running when the firmware
    // upgrade starts the target should respons to all `0x00` requestes with 
    // `RESP_OK` if the bus is IDLE
    if (bus_state == STATE_IDLE) {
        if (data == 0) {
            send_response(RESP_OK);
        }
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

/// Timeout interface handler will occur excactly 1 second after the timer has 
/// been reloaded. To avoid this the software must either reload the timer or
/// stop it
void timer0_ch0_handler(void) {
    // Clear timer flags
    (void)TIMER0->channel[0].SR;

    printl("Bootloader timeout");

    timeout_stop();
    bus_state = STATE_IDLE;
}
