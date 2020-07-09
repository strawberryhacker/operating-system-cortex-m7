/// Copyright (C) StrawberryHacker

#include "sd_protocol.h"
#include "print.h"
#include "hardware.h"
#include "clock.h"
#include "panic.h"
#include "memory.h"

/// When coding this part the SD protocol specification will be handy
/// It can be found here 
/// http://academy.cba.mit.edu/classes/networking_communications/SD/SD.pdf
///
/// For understanding the initialization sequence take a look at the card 
/// identification process at page 28 and the data transfer mode state diagram 
/// at page 35.
///
/// Below is a summary of the different bus-speeds
/// (1) Default speed - 3.3V - up to 25 MHz
/// (2) High speed    - 3.3V - up to 50 MHz
/// (3) SDR12 UHS-I   - 1.8V - up to 25 MHz
/// (4) SDR25 UHS-I   - 1.8V - up to 50 MHz
/// (5) SDR50 UHS-I   - 1.8V - up to 100 MHz
///
/// An SD card has allways a six-wire interface; clock, cmd and 4 data lines. 
/// An SD card belongs to a capacity class. Standard capacity (SDSC) supports 
/// up to 2 GB of memory. High capacity (SDHC) supports up to 32 GB of memory.
/// Extended capacity (SDXC) supports up to 2 TB of memory. 
///
/// Command & response
/// A CMD token consist of a start bit, tx bit, payload, crc and stop bit. The
/// total length is 48 bits. The response token is sneaky and may appear in 
/// different forms. It is either 48 bit or 136 bit. The 136 bit response also
/// includes a CRC. All commands and responses are transmitted MSB first on the
/// CMD line. 
///
/// Data
/// Data is either transmitted on 1 or 4 data lanes. Start bit, stop bit and CRC
/// is calulated and sendt for every of the data lines used. There are two 
/// types of data packets. Usual data (8-bit) are sent LSByte first, and MSBit
/// first whithin the byte. Wide data (SD memory register) are transmitted 
/// MSbit first. 
///
/// Registers
/// CID - card ID number 128 bits
/// RCA - relative card address, dynamicall suggested by the card and approved
///       by the host during initialization 16 bits
/// CSD - card spesific data contains info about operating conditions 128 bits
/// SCR - SD configuration register contains info about special features 64 bit
/// OCR - operating condition register 32 bit
/// SSR - SD status contains info about the cards proprietary features
/// CSR - card status register 
///
/// An SD card can be in one of many states and operating modes. Three
/// operating modes exist; inactive, card identification mode and data transfer
/// mode. The card identification mode has the following states; idle, ready
/// and identification. Data trasfer mode has the following states; stand-by,
/// transfer, sending-data, receiving-data, programming state and disconnected. 
/// The card identification stage should happend with clock rate f_od = 400 KHz

/// This CPU only supports one MMC slot. Therefore it is declared globally and 
/// automatically updated by the SD stack
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

/// Resets all connected cards to IDLE state
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

/// Asks the card to send its CID number. This is manatory. After a successful
/// CMD2 the card will enter identification state. The functions returns the 
/// 16-byte CID register
static u8 sd_exec_cmd_2(u8* cid) {
    u32 cmd = SD_RESP_2 | MMC_CMD_OPEN_DRAIN | 2;
    u32 arg = 0;

    if (mmc_send_cmd(cmd, arg, 1) == 0) {
        return 0;
    }
    mmc_read_resp136(cid);

    // Extract the product name at offset [103:64]
    for (u8 i = 0; i <= 5; i++) {
        slot_1.cid_name[i] = (char)cid[12 - i];
    }
    return 1;
}

/// Asks the card to publish a new relcative card address (RCA). This will 
/// automatically update the slot 1 structure because only one card is 
/// supported. Therefore the first published address will allways be valid.
static u8 sd_exec_cmd_3(void) {
    u32 cmd = SD_RESP_6 | MMC_CMD_OPEN_DRAIN | 3;
    u32 arg = 0;

    if (mmc_send_cmd(cmd, arg, 1) == 0) {
        return 0;
    }
    u32 resp = mmc_read_resp48();

    // Get the relative card address field
    slot_1.rca = (resp >> 16) & 0xFFFF;
    return 1;
}

/// Sends the supplied voltage to the card. It will echo back the voltage and
/// the check pattern if it supports the voltage. This functions returns `1` if
/// the supplied voltage is supported. If not it returns `0`. This means one of 
/// two things; either the card does not support the voltage, or the card is 
/// Ver1.X SD card. In the first case the ACMD41 should be called with HCS equal
/// zero. If HCS is zero Ver2.00 or later SD card will not respond to this. 
static u8 sd_exec_cmd_8(u8 vhs) {
    u32 cmd = SD_RESP_7 | MMC_CMD_OPEN_DRAIN | 8;

    // Send the voltage supplied
    u32 arg = ((vhs & 0b1111) << 8) | 0b10101010;

    // If Ver1.X or voltage mismatch this will return `0` because of timeout
    if(mmc_send_cmd(cmd, arg, 1) == 0) {
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
    return 0;
}

/// Switch to high speed SDR-25. NOTE the user must check the SD version before 
/// using this command. This command is NOT supported by version 1.0 and 1.01
u8 sd_exec_cmd_6(void) {
    slot_1.high_speed = 0;

    u8 buffer[64];
    memory_fill(buffer, 0, 64);


    u32 cmd = SD_RESP_1 | MMC_CMD_SINGLE | MMC_CMD_START_DATA | MMC_CMD_READ | 6;
    u32 arg = (1 << 31) | (0xF << 12) | (0xF << 8) | (0xF << 4) | (1 << 0);

    if (mmc_send_adtc(cmd, arg, 64, 1, 1) == 0) {
        return 0;
    }

    mmc_read_data_reverse(buffer, 16);
    u32 resp = mmc_read_resp48();

    // Check the response
    if (resp & (1 << 7)) {
        return 0;
    }
    if ((buffer[35] << 8) | buffer[34]) {
        print("Card is busy\n");
        return 0;
    }
    if ((buffer[47] & 0xF) == 0xF) {
        print("High speed error\n");
        slot_1.high_speed = 0;
        return 1;
    }

    slot_1.high_speed = 1;
    return 1;
}

/// Toggles the SD card between the stand-by and transfer state
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
/// command
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

/// Sends the host capacity support and asks the card for its OCR register. 
/// This function will query the card for its OCR register and determine if the
/// card supports high capacity. If the card responds it returns `1`, if not
/// the card will go into inactive state. It takes in the result from CMD8.
/// SD spec. page 27, 29 and 161
static u8 sd_exec_acmd_41(u8 cmd8_resp) {
    u32 cmd = SD_RESP_3 | MMC_CMD_OPEN_DRAIN | 41;

    // Bit 30 indicated that the host supports high capacity SD cards. Bit 
    // 0-32 indicates the VDD range specified at page 161 in the SD protocol. 
    // The host supports voltages from 3.2V to 3.4V
    u32 arg = (0b11 << 20);

    // The host should not set the HCS bit to one if the CMD8 failed. Any card
    // not responding to CMD8 wont consider the HCS field. 
    if (cmd8_resp) {
        arg |= (1 << 30);
    }

    // The host should poll the card with ACMD41 untill the card is NOT busy,
    // that is; the busy bit (bit 31) is set in the OCR register. All cards must
    // respond within 1 second
    u32 timeout = 1000;
    while (timeout--) {
        if (sd_exec_cmd_55() == 0) {
            return 0;
        }
        if (mmc_send_cmd(cmd, arg, 0) == 0) {
            return 0;
        }

        // Read the response and test bit 31
        u32 resp = mmc_read_resp48();

        if (resp & (1 << 31)) {
            // Card is not busy anymore            
            // Check if the card supports the supplied voltage
            if ((resp & (0b11 << 15)) != (0b11 << 15)) {
                return 0;
            }
            // The CCS bit (bit 30) indicates if the SD card supports high 
            // capacity
            if (resp & (1 << 30)) {

                // CCS bit set indicating a high capacity SD card; SDHC or SDXC
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

/// Wait for the card to be ready
static u8 sd_exec_cmd_13(void) {
    u32 timeout = 200000;
    u32 status;

    do {
        u32 cmd = SD_RESP_1 | 13;
        u32 arg = slot_1.rca << 16;

        if (!mmc_send_cmd(cmd, arg, 1)) {
            return 0;
        }
        
        if (timeout-- <= 1) {
            return 0;
        }

        status = mmc_read_resp48();
    } while (!(status & (1 << 8)));

    return 1;
}

/// Retrieves the SCR register which contains information about the additional 
/// features. This includes non-supported commands, bus width and physical 
/// layer version.
static u8 sd_exec_acmd_51(void) {
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

/// Defines the bus width to be used in following transfers. The user must 
/// check if the bus width is supported in the SCR register
u8 sd_exec_acmd_6(u8 bus_width) {
    // The next command is an application command
    if (sd_exec_cmd_55() == 0) {
        return 0;
    }

    u32 cmd = SD_RESP_1 | 6;
    u32 arg = bus_width & 0b11;

    if (mmc_send_cmd(cmd, arg, 1) == 0) {
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

    // SD cards can have two different CSD structures. Which one is defined by
    // the two last bits of the CSD. Generally the CAD Version 1.0 only applies
    // to SDSC cards, while the CSD Version 2.0 apply to SDHC cards
    u8 csd_structure = (csd[15] >> 6) & 0b11;

    if (csd_structure == 0) {
        // CSD Version 1.0
        u32 c_size      = ((csd[7] >> 6) & 0b11) | (csd[8] << 2) | 
                          ((csd[9] & 0b11) << 10);
        u32 c_size_mult = (((csd[6] & 0b11) << 1) | ((csd[5] >> 7) & 0b1));
        u32 read_bl_len = (csd[10] & 0b1111);

        slot_1.kib_size = ((c_size + 1) * (1 << (c_size_mult + 2)) *
                        (1 << read_bl_len)) / 1000;
        
        // This card should be a SDSC card
        if (slot_1.high_capacity) {
            print("Warning\n");
        }

    } else if (csd_structure == 1) {
        // CSD Version 2.0
        u32 c_size = ((csd[8] & 0x3F) << 16) | (csd[7] << 8) | csd[6];
        slot_1.kib_size = c_size * 512;

        // This card should be a SDHC card
        if (slot_1.high_capacity == 0) {
            print("Warning\n");
        }
    } else {
        panic("CSD structure error");
    }
}

/// Performs the SD protocol initialization sequence and brings the card to the
/// transfer state
void sd_protocol_init(void) {
    print("Starting SD protocol init sequence...\n");

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

    // The RCA of the CMD55 must be zero in idle state before the card has
    // proposed a RCA. Therefore it must be initialized to zero.
    slot_1.rca = 0;

    // Start the card identification stage
    // Send the 75 clock cylces initialization sequence
    if (sd_boot() == 0) {
        panic("SD boot failed");
    }
    // Send CMD0
    if (!sd_exec_cmd_0()) {
        panic("CMD0 failed");
    }
    // Send CMD8 to check operating conditions 2.7V - 3.6V
    u8 cmd8_status = sd_exec_cmd_8(0b0001);

    if (cmd8_status == 0) {
        printl("Warning - CMD8");
    }
    // Read capacity support and SD card protocol version 
    if (sd_exec_acmd_41(cmd8_status) == 0) {
        panic("ACMD41 failed");
    }
    print("SDHC support: %d\n", slot_1.high_capacity);

    // Get the CID register and place the card into identification mode
    u8 cid[16];
    if (sd_exec_cmd_2(cid) == 0) {
        panic("Can't retrieve CID register");
    }
    // Query the card for a new RCA. Since the CPU only supports one slot it
    // doesn't need to ask more than once
    if (sd_exec_cmd_3() == 0) {
        panic("Can't retrieve RCA address");
    }

    // The card is in date transfer mode
    // Get the CSD including card size and block length
    u8 csd[16];
    if (sd_exec_cmd_9(csd) == 0) {
        panic("Retrieve CSD failed");
    }
    // Decode the CSD register and update the block size and capacity
    csd_decode(csd);
    print("Size: %d\n", slot_1.kib_size);

    // Select the card in slot 1 and put it in transfer state
    if (sd_exec_cmd_7() == 0) {
        panic("CMD7 failed");
    }

    // Check if the SD card can use 4 bit bus
    if (sd_exec_acmd_51() == 0) {
        panic("ACMD51 failed");
    }
    print("4-bit support: %d\n", slot_1.four_bit_bus_support);
    
    // ACMD6 - if appropriate change the bus width (0b00 for 1-bit and 0b10 
    // for 4-bit)
    if (slot_1.four_bit_bus_support) {
        if (sd_exec_acmd_6(0b10) == 0) {
            panic("Can't set 4-bit bus width");
        }
        mmc_set_bus_width(MMC_4_LANES);
    }

    // Try to switch to high-speed mode if supported. This used CMD6 and 
    // requires SD card Version 1.10 or later
    if (slot_1.past_v1_10) {
        if (sd_exec_cmd_6() == 0) {
            panic("CMD6 failed");
        }
        print("High-speed enabled: %d\n", slot_1.high_speed);

        // Update the bus speed
        if (slot_1.high_speed) {
            mmc_enable_high_speed();
            mmc_set_bus_freq(50000000);
        } else {
            mmc_set_bus_freq(25000000);
        }
    }
    printl("SD card ready\n");
}

u8 sd_read(u32 sector, u32 count, u8* buffer) {
    u32 cmd = 0;
    u32 arg = 0;

    if ((sector * 512) > slot_1.block_count) {
        panic("Block size wrong");
    }

    u32* buffer_word = (u32 *)buffer;

    for (u32 i = 0; i < count; i++) {

        // Check if the card is ready
        if (!sd_exec_cmd_13()) {
            panic("Card is not ready");
        }

        cmd = MMC_CMD_SINGLE | MMC_CMD_START_DATA | MMC_CMD_READ | SD_RESP_1 | 17;

        // The argument is dependent upon the card capacity
        if (slot_1.high_capacity) {
            arg = sector + i;
        } else {
            arg = (sector + i) * 512;
        }

        mmc_send_adtc(cmd, arg, 512, 1, 1);

        for (u8 j = 0; j < 128; j++) {
            *buffer_word++ = mmc_read_data();
        }

        // Check for errors
        u32 status = mmc_read_resp48();
        if (status & 0xFFF80000) {
            panic("Write error");
        }
    }
    return 1;
}

u8 sd_write(u32 sector, u32 count, const u8* buffer) {
    u32 cmd = 0;
    u32 arg = 0;

    const u32* buffer_word = (const u32 *)buffer;

    for (u32 i = 0; i < count; i++) {
        cmd = MMC_CMD_SINGLE | MMC_CMD_START_DATA | MMC_CMD_WRITE | SD_RESP_1 | 24;

        // The argument is dependent upon the card capacity
        if (slot_1.high_capacity) {
            arg = sector + i;
        } else {
            arg = (sector + i) * 512;
        }

        mmc_send_adtc(cmd, arg, 512, 1, 1);

        for (u8 j = 0; j < 128; j++) {
            mmc_write_data(*buffer_word++);
        }

        // Check for errors
        u32 status = mmc_read_resp48();
        if (status & 0xFFF80000) {
            panic("Read error");
        }

        // Wait for the card to return not busy
        while (!(mmc_read_status() & (1 << 5)));
    }
    return 1;
}

u8 sd_dma_write(u32 sector, u32 count, const u8* buffer) {
    return 1;
}

u8 sd_dma_read(u32 sector, u32 count, u8* buffer) {
    return 1;
}
