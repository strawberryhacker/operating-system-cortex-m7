#include "mmc.h"
#include "hardware.h"
#include "panic.h"
#include "print.h"

void mmc_init(void) {

    // Using the default configuration
    MMC->MR &= ~0xFF00;

    // Turn on read and write proof. This guaranties data integrity, not 
    // bandwidth,  by stopping the MMC clock if the internal FIFO is full
    MMC->MR |= (1 << 11) | (1 << 12);

    // Set the timeouts
    MMC->DTOR  = (7 << 4) | (3 << 0);
    MMC->CSTOR = (7 << 4) | (3 << 0);
    
    // Configuration register - enable FIFO mode and flow error reset
    MMC->CFG = (1 << 0) | (1 << 4);
}

void mmc_reset(void) {
    MMC->CR = (1 << 7);
}

void mmc_enable(void) {
    MMC->CR = 0b1;
}

void mmc_disable(void) {
    MMC->CR = 0b10;
}

/// The peripheral clock MUST run at 150 MHz
void mmc_set_bus_freq(u32 frequency) {
    u8 div = 0;
    u8 odd = 0;

    // The bus clock is 2 x DIV + ODD + 2
    if (2 * frequency < 150000000) {
        u32 prescaler = 150000000 / frequency - 2;
        
        if (150000000 % frequency) {
            prescaler++;
        }

        div = prescaler >> 1;
        odd = prescaler & 0b1;
    }
    MMC->MR &= ~((1 << 16) & 0xFF);
    MMC->MR |= (odd << 16) | div;
}

/// The MMC interface only support 1 and 4 bit busses
void mmc_set_bus_width(enum mmc_bus_width width) {
    MMC->SDCR &= ~(0b11 << 6);
    MMC->SDCR |= (width << 6);
}

void mmc_enable_high_speed(void) {
    MMC->CFG |= (1 << 8);
}

void mmc_disable_high_speed(void) {
    MMC->CFG &= ~(1 << 8);
}

void mmc_dma_enable(enum mmc_dma_chunk chunk) {
    MMC->DMA &= ~(0b111 << 4);
    MMC->DMA |= (chunk << 4);
    MMC->DMA |= (1 << 8);
}

void mmc_dma_disable(void) {
    MMC->DMA &= ~(1 << 8);
}

u8 mmc_send_cmd(u32 cmd, u32 arg, u8 check_crc) {
    MMC->ARG = arg;
    MMC->CMD = cmd;

    u32 status;
    do {
        status = MMC->SR;

        // Check for the following errors
        // - Response index error
		// - Response direction error
		// - Response end bit error
		// - Response time out error
		// - Completion signal timeout
        if (status & ((1 << 16) | (1 << 17) | (1 << 19) | (1 << 20) |
            (1 << 22))) {
            
            panic("MMC send command error");
            return 0;
        }
        if (check_crc && (status & (1 << 18))) {
            panic("MMC send command CRC error");
            return 0;
        }
    } while (!(status & 0b1));

    // If the expected response is R1b, the card is busy and should not be given
    // any more work util ready
    if (((cmd >> 6) & 0b11) == 3) {
        // Response is R1b
        u32 timeout = 0xFFFFFFFF;
        do {
            if (timeout-- <= 1) {
                panic("MMC command timeout");
            }
            status = MMC->SR;
        } while (!(status & (1 << 5)) && !(status & (1 << 4))); 
    }
    return 1;
}

u8 mmc_send_adtc(u32 cmd, u32 arg, u32 block_size, u32 block_count, u8 check_crc) {

    // Check if force byte transfer has to be used
    if (block_size & 0b11) {
        MMC->MR |= (1 << 13);
    } else {
        MMC->MR &= ~(1 << 13);
    }

    // Check if the MMC has to force byte transfer
    if (((cmd >> 19) & 0b111) == 4) {
        MMC->BLKR = (block_size % 512) & 0xFFFF;
    } else {
        MMC->BLKR = (block_size << 16) & (block_count & 0xFFFF);
    }
    
    // Initiate transfer
    MMC->ARG = arg;
    MMC->CMD = cmd;
    u32 time_out = 1000;
    u32 status;
    do {
        status = MMC->SR;
        if (time_out-- > 1){
            panic("Timeout");
        }
    } while (!(status & 0b1));

    // Check error flags
    if (status & 0x21FB0000) {
        print("Reg: %32b\n", status & 0x21FB0000);
        panic("CMD failed");
        return 0;
    }

    if (check_crc) {
        // Check for CRC error
        if (status & (1 << 18)) {
            panic("CMD failed CRC error");
            return 0;
        }
    }

    // If the expected response is R1b, the card is busy and should not be given
    // any more work util ready
    if (((cmd >> 6) & 0b11) == 3) {
        // Response is R1b
        u32 timeout = 0xFFFFFFFF;
        do {
            if (timeout-- <= 1) {
                panic("MMC command timeout");
            }
            status = MMC->SR;
        } while (!(status & (1 << 5)) && !(status & (1 << 4))); 
    }
    return 1;
}

u32 mmc_read_resp48(void) {
    return MMC->RESP[0];
}

void mmc_read_resp136(u8* buffer) {
    u8 byte_reverse[16];

    for (u8 i = 0; i < 4; i++) {
        u32 resp = MMC->RESP[i];

        byte_reverse[4 * i]   = (resp >> 24) & 0xFF;
		byte_reverse[4 * i + 1] = (resp >> 16) & 0xFF;
		byte_reverse[4 * i + 2] = (resp >> 8) & 0xFF;
		byte_reverse[4 * i + 3] = resp & 0xFF;
    }

    for (u8 i = 0; i < 16; i++) {
        *buffer++ = byte_reverse[15 - i];
    }
}

u32 mmc_read_data(void) {
    while (!(MMC->SR & (1 << 1)));
    return MMC->RDR;
}

void mmc_read_data_reverse(u8* buffer, u32 word_count) {
    for (uint8_t i = 0; i < word_count; i++) {
        // Wait for data to appear in the RX register
        while (!(MMC->SR & (1 << 1)));

        u32 reg = MMC->RDR;

        // Grab a pointer to the last byte
        buffer += word_count * 4;

        *buffer-- = (reg >> 24) & 0xFF;
        *buffer-- = (reg >> 16) & 0xFF;
        *buffer-- = (reg >> 8) & 0xFF;
        *buffer-- = reg & 0xFF;
    }
}

void mmc_write_data(u32 data) {
    // Wait for the TX ready flag
    while (!(MMC->SR & (1 << 2)));
    MMC->TDR = data;
}
