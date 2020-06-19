#include "sd_protocol.h"
#include "debug.h"
#include "hardware.h"
#include "mmc.h"
#include "clock.h"
#include "panic.h"

/// When implementing features the SD protocol will by useful. It can be found
/// here http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
/// and all the page numbers is taken from this datasheet.

/// The CPU only supports one slot
static struct sd_card card = { 0 };

/// Initialize the SD card by sending the 74-cylce init sequence
u8 sd_boot(void) {
    u32 cmd = MMC_CMD_INIT | MMC_CMD_OPEN_DRAIN;
    u32 arg = 0;

    u8 status = mmc_send_cmd(cmd, arg, 1);

    if (status) {
        return 1;
    } else {
        return 0;
    }
}

/// Sets the sd card in IDLE state
/// arg  - don't care
/// resp - none
u8 sd_exec_cmd_0(void) {
    u32 cmd = MMC_CMD_OPEN_DRAIN | 0;
    u32 arg = 0;

    u8 status = mmc_send_cmd(cmd, arg, 1);

    if (status) {
        return 1;
    } else {
        return 0;
    }
}

/// Asks the card to send the CID numbers
/// arg  - don't care
/// resp - R2
u8 sd_exec_cmd_2(u8* cid) {
    u32 cmd = SD_RESP_2 | MMC_CMD_OPEN_DRAIN | 2;
    u32 arg = 0;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }
    mmc_read_resp136(cid);

    // Extract the device name
    for (u8 i = 0; i <= 5; i++) {
        card.cid_name[i] = (char)cid[12 - i];
    }
    return 1;
}

/// Get the relative card address (RCA)
/// arg  - don't care
/// resp - R6
u8 sd_exec_cmd_3(void) {
    u32 cmd = SD_RESP_6 | MMC_CMD_OPEN_DRAIN | 3;
    u32 arg = 0;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }
    u32 resp = mmc_read_resp48();

    // Get the relative card address field
    card.rca = (resp >> 16) & 0xFFFF;
    return 1;
}

/// Check if the card can operate with the given voltage
/// arg  - [11..8] supply voltage VHS
///        [7..0]  check pattern
/// resp - R7
u8 sd_exec_cmd_8(u8 vhs) {
    u32 cmd = SD_RESP_7 | MMC_CMD_OPEN_DRAIN | 8;

    // Send the voltage supplied 2.7V - 3.6V
    u32 arg = ((vhs & 0b1111) << 8) | 0b10101010;

    if(!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }

    // Read the SD card response
    u32 resp = mmc_read_resp48();

    // The SD card should echo the checkpattern and operating voltage if
    // supported. If SD v1.0 the card will not responde. If SD v2.0 and voltage
    // mismatch the card is unusable. 
    if ((resp & 0b111111111111) == arg) {
        return 1;
    }
    panic("Operating voltage not supported");
    return 0;
}

/// Tells the card the next command to be sent is an application specific
/// command (ACMDx). 
/// arg  - don't care
/// resp - R1
u8 sd_exec_cmd_55(void) {
    u32 cmd = SD_RESP_1 | 55;
    u32 arg = 0;

    u8 status = mmc_send_cmd(cmd, arg, 1);

    if (status) {
        return 1;
    } else {
        return 0;
    }
}

/// Send host capacity support and asks the card for its operating condition 
/// register (OCR).
/// arg  - [30] HCS
///      - [28] XPC
///      - [24] S18R
///      - [23..0] VDD voltage window
/// resp - R1
u8 sd_exec_acmd_41(void) {
    u32 cmd = SD_RESP_3 | MMC_CMD_OPEN_DRAIN | 41;

    // Bit 30 indicated that the host supports high capacity SD cards. Bit 
    // 0-32 indicates the VDD range specified at page 161 in the SD protocol
    u32 arg = (1 << 30) | (0b111111 << 15);

    // The card will typicall not be ready when the first command is sent. For
    // a low capacity SD card it can take many retries to receive a valid bit 31
    u32 timeout = 1000;
    while (timeout--) {
        if (!sd_exec_cmd_55()) {
            return 0;
        }
        if (!mmc_send_cmd(cmd, arg, 0)) {
            return 0;
        }

        u32 resp = mmc_read_resp48();
        if (resp & (1 << 31)) {
            if (resp & (1 << 30)) {
                // HCS bit set indicated high capacity SD card
                card.high_capacity = 1;
            } else {
                // Standard capacity SD card
                card.high_capacity = 0;
            }
            return 1;
        }
    }
    return 0;
}

void sd_protocol_init(void) {
    debug_print("Starting SD protocol init sequence...\n");

    // Enable the MMC clock
    peripheral_clock_enable(18);

    // Start up the MMC interface
    mmc_reset();
    mmc_init();

    // The SD protocol initialization should happend on 1 lane at 400 kHz
    mmc_set_bus_width(MMC_1_LANE);
    mmc_set_bus_freq(400000);

    mmc_enable();

    // Boot SD card
    if (!sd_boot()) {
        panic("SD boot failed");
    }

    // Send CMD0
    if (!sd_exec_cmd_0()) {
        panic("CMD0 failed");
    }

    // Card is in IDLE state

    // Send CMD8 to check operating conditions 2.7V - 3.6V
    if (!sd_exec_cmd_8(0b0001)) {
        panic("CMD8 failed");
    }

    // Card should return response, if not it is not supported (SD Ver. 1)

    // SD Ver. 2.0

    // Check response

    // Wait for card to be ready

    // Read capacity support
    if (!sd_exec_acmd_41()) {
        panic("ACMD41 failed");
    }

    // CMD11 is not in use because the host does not require to switch to 1.8V

    // CMD2
    u8 cid[16];
    if (!sd_exec_cmd_2(cid)) {
        panic("Can't retrieve CID register");
    }
    debug_print("Card detected with ID - ");
    for (u8 i = 0; i < 6; i++) {
        debug_print("%c", card.cid_name[i]);
    }
    debug_print("\n");

    // CMD3
    if (!sd_exec_cmd_3()) {
        panic("Can't retrieve RCA address");
    }
    debug_print("RCA: %4h\n", card.rca);

    // The card is in date transfer mode

    // CMD9

    // CMD7

    // ACMD51

    // ACMD6


}

void sd_write(u32 sector, u32 size, u8* buffer) {

}

void sd_read(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_write(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_read(u32 sector, u32 size, u8* buffer) {

}
