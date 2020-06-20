#include "sd_protocol.h"
#include "debug.h"
#include "hardware.h"
#include "mmc.h"
#include "clock.h"
#include "panic.h"
#include "memory.h"

/// When coding this part the SD protocol specification will prove useful.
/// It can be found here 
/// http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
///
/// For understanding the initialization sequence take a look at the card 
/// identification process at page 28 and the data transfer mode state diagram 
/// at page 35

/// This CPU only supports one MMC slot. Therefore it is declared globally and 
/// automatically updated be the SD stack
static struct sd_card slot_1 = { 0 };

/// Initialize the SD card by sending the 74-cylce init sequence
static u8 sd_boot(void) {
    u32 cmd = MMC_CMD_INIT | MMC_CMD_OPEN_DRAIN;
    u32 arg = 0;

    u8 status = mmc_send_cmd(cmd, arg, 1);

    if (status) {
        return 1;
    } else {
        return 0;
    }
}

/// Sets the SD card in IDLE state
/// arg  - don't care
/// resp - none
static u8 sd_exec_cmd_0(void) {
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
static u8 sd_exec_cmd_2(u8* cid) {
    u32 cmd = SD_RESP_2 | MMC_CMD_OPEN_DRAIN | 2;
    u32 arg = 0;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }
    mmc_read_resp136(cid);

    // Extract the device name
    for (u8 i = 0; i <= 5; i++) {
        slot_1.cid_name[i] = (char)cid[12 - i];
    }
    return 1;
}

/// Get the relative card address (RCA)
/// arg  - don't care
/// resp - R6
static u8 sd_exec_cmd_3(void) {
    u32 cmd = SD_RESP_6 | MMC_CMD_OPEN_DRAIN | 3;
    u32 arg = 0;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }
    u32 resp = mmc_read_resp48();

    // Get the relative card address field
    slot_1.rca = (resp >> 16) & 0xFFFF;
    return 1;
}

/// Check if the card can operate with the given voltage
/// arg  - [11..8] supply voltage VHS
///        [7..0]  check pattern
/// resp - R7
static u8 sd_exec_cmd_8(u8 vhs) {
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

/// Switches the card function
u8 sd_exec_cmd_6(void) {
    slot_1.high_speed = 0;

    u8 buffer[64];
    memory_fill(buffer, 0, 64);

    u32 cmd = SD_RESP_1 | MMC_CMD_SINGLE | MMC_CMD_START_DATA | MMC_CMD_READ | 6;
    u32 arg = (1 << 31) | (0xF << 12) | (0xF << 8) | (0xF << 4);

    if (!mmc_send_adtc(cmd, arg, 64, 1, 1)) {
        return 0;
    }

    mmc_read_data_reverse(buffer, 16);
    u32 resp = mmc_read_resp48();

    if ((buffer[35] << 8) | buffer[34]) {
        debug_print("Card is busy\n");
        return 0;
    }
    if ((buffer[47] & 0xF) == 0xF) {
        debug_print("High speed error\n");
        return 1;
    }
    if (resp & 0xFFF80000) {
        return 0;
    }

    slot_1.high_speed = 1;
    return 1;
}

/// Toggles the SD card between the stand-by and transfer state
/// arg  - [31..16] RCA
/// resp - R1b
static u8 sd_exec_cmd_7(void) {
    u32 cmd = SD_RESP_1b | 7;
    u32 arg = slot_1.rca << 16;

    u8 status = mmc_send_cmd(cmd, arg, 1);

    if (status) {
        return 1;
    } else {
        return 0;
    }
}

/// Retrieve the card-specific data (CSD) from the card
/// arg  - [31..16] RCA
/// resp - R2
static u8 sd_exec_cmd_9(u8* csd) {
    u32 cmd = SD_RESP_2 | 9;
    u32 arg = slot_1.rca << 16;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }

    mmc_read_resp136(csd);
    return 1;
}

/// Tells the card the next command to be sent is an application specific
/// command (ACMDx). 
/// arg  - don't care
/// resp - R1
static u8 sd_exec_cmd_55(void) {
    u32 cmd = SD_RESP_1 | 55;
    u32 arg = slot_1.rca << 16;

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
static u8 sd_exec_acmd_41(void) {
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
                slot_1.high_capacity = 1;
            } else {
                // Standard capacity SD card
                slot_1.high_capacity = 0;
            }
            return 1;
        }
    }
    return 0;
}

/// Retrieves the SCR register which contains information about the additional 
/// features. This includes non-supported commands, bus width and physical 
/// layer version.
u8 sd_exec_acmd_51(void) {
    // The next command will be an application command
    if (!sd_exec_cmd_55()) {
        return 0;
    }

    u32 cmd = SD_RESP_1 | MMC_CMD_READ | MMC_CMD_START_DATA | MMC_CMD_BYTE | 51;
    u32 arg = 0;

    if (!mmc_send_adtc(cmd, arg, 8, 1, 1)) {
        return 0;
    }

    u8 csr[8];
    mmc_read_data_reverse(csr, 2);

    // Check if the card supports 4 bit bus width
    if (((csr[6] >> 2) & 0b1) == 0b1) {
        slot_1.four_bit_bus_support = 1;
    } else {
        slot_1.four_bit_bus_support = 0;
    }

    // Check if the SD card Version is higher than Version 1.0 and Version 1.01
    // This has to do with the command set implemented. For example, a 
    // version 1.0 SD card does not support CMD6 and therefore not high-speed 
    // either
    if (csr[7] & 0b1111) {
        slot_1.past_v1_10 = 1;
    }

    return 1;
}

/// Defines the bus width
u8 sd_exec_acmd_6(u8 bus_width) {
    // The next command is an application command
    if (!sd_exec_cmd_55()) {
        return 0;
    }

    u32 cmd = SD_RESP_1 | 6;
    u32 arg = bus_width & 0b11;

    if (!mmc_send_cmd(cmd, arg, 1)) {
        return 0;
    }

    // Read the response register and check the status. The 32-bit status field
    // is described at page 103 in the specification
    u32 status = mmc_read_resp48();

    if (status & 0xFFF80000) {
        return 0;
    }
    return 1;
}

/// Extracts information from the CSD register and updates the `slot_1` object
static void csd_decode(const u8* csd) {
    u8 csd_structure = (csd[15] >> 6) & 0b11;

    // SD cards can have two different CSD structures. Which one is defined by
    // the two last bits of the CSD. Generally the CAD Version 1.0 only applies
    // to SDSC cards, while the CSD Version 2.0 apply to SDHC cards

    if (csd_structure == 0) {
        // CSD Version 1.0
        u32 c_size      = ((csd[7] >> 6) & 0b11) | (csd[8] << 2) | 
                          ((csd[9] & 0b11) << 10);
        u32 c_size_mult = (((csd[6] & 0b11) << 1) | ((csd[5] >> 7) & 0b1));
        u32 read_bl_len = (csd[10] & 0b1111);

        slot_1.k_size = ((c_size + 1) * (1 << (c_size_mult + 2)) *
                        (1 << read_bl_len)) / 1000;
        
        // This card should be a SDSC card
        if (slot_1.high_capacity) {
            debug_print("Warning\n");
        }

    } else if (csd_structure == 1) {
        // CSD Version 2.0
        u32 c_size = ((csd[8] & 0x3F) << 16) | (csd[7] << 8) | csd[6];
        slot_1.k_size = c_size * 512;

        // This card should be a SDHC card
        if (!slot_1.high_capacity) {
            debug_print("Warning\n");
        }
    } else {
        panic("CSD structure error");
    }
}

/// Performs the SD protocol initialization sequence and brings the card to the
/// transfer state
void sd_protocol_init(void) {
    debug_print("Starting SD protocol init sequence...\n");

    // Enable the MMC clock
    peripheral_clock_enable(18);

    // Start up the MMC interface
    mmc_reset();
    mmc_init();

    // The SD protocol initialization should not use higher frequency than 
    // 400 kHz. Additionally the card does not nessecary support 4-bit bus width
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
    debug_print("SDHC support: %d\n", slot_1.high_capacity);

    // CMD11 is not in use because the host does not require to switch to 1.8V

    // CMD2
    u8 cid[16];
    if (!sd_exec_cmd_2(cid)) {
        panic("Can't retrieve CID register");
    }
    debug_print("Card detected with ID - ");
    for (u8 i = 0; i < 6; i++) {
        debug_print("%c", slot_1.cid_name[i]);
    }
    debug_print("\n");

    // CMD3
    if (!sd_exec_cmd_3()) {
        panic("Can't retrieve RCA address");
    }
    debug_print("RCA: %4h\n", slot_1.rca);

    // The card is in date transfer mode

    // CMD9
    u8 csd[16];
    if (!sd_exec_cmd_9(csd)) {
        panic("Retrieve CSD failed");
    }

    csd_decode(csd);

    debug_print("Size: %d\n", slot_1.k_size);

    // CMD7
    if (!sd_exec_cmd_7()) {
        panic("CMD7 failed");
    }

    // ACMD51
    while (1);
    if (!sd_exec_acmd_51()) {
        panic("ACMD51 failed");
    }
    debug_print("4-bit support: %d\n", slot_1.four_bit_bus_support);
    
    // ACMD6 - if appropriate change the bus width (0b00 for 1-bit and 0b10 
    // for 4-bit)
    if (slot_1.four_bit_bus_support) {
        if (!sd_exec_acmd_6(0b10)) {
            panic("Can't set 4-bit bus width");
        }
        mmc_set_bus_width(MMC_4_LANES);
    }

    // Try to switch to high-speed mode if supported. This used CMD6 and 
    // requires SD card Version 1.10 or later
    if (slot_1.past_v1_10) {
        if (!sd_exec_cmd_6()) {
            panic("CMD6 failed");
        }
        debug_print("HSS: %d\n", slot_1.high_speed);
    }
    mmc_set_bus_freq(50000000);
}

void sd_write(u32 sector, u32 size, u8* buffer) {

}

void sd_read(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_write(u32 sector, u32 size, u8* buffer) {

}

void sd_dma_read(u32 sector, u32 size, u8* buffer) {

}
