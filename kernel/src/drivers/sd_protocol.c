#include "sd_protocol.h"
#include "debug.h"
#include "hardware.h"
#include "mmc.h"

/// The CPU only supports one slot
static struct sd_card card = { 0 };

/// These functions implements a subset og the SD protocol command set
void sd_exec_cmd_0(void);

void sd_protocol_init(void) {
    debug_print("Starting SD protocol init sequence...\n");

    // Start up the MMC interface
    mmc_reset();
    mmc_init();

    // Boot SD card

    // Send CMD0

    // Card is in IDLE state

    // Send CMD8 to check operating conditions

    // Card should return response, if not it is not supported (SD Ver. 1)

    // SD Ver. 2.0

    // Check response

    // Wait for card to be ready

    // Read capacity support

    // CMD11

    // CMD2

    // CMD3
}

void sd_write(u32 sector, u32 size, u8* buffer) {

}

void sd_read(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_write(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_read(u32 sector, u32 size, u8* buffer) {

}
