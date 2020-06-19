#ifndef SD_PROTOCOL_H
#define SD_PROTOCOL_H

#include "types.h"
#include "mmc.h"

struct sd_card {
    u32 rca;
    char cid_name[6];
    u8  initialized : 1;
    u8  high_capacity : 1;
};

// R1
// 48-bit response
#define SD_RESP_1  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48

// R1b
// same as R1 but with busy on the data line
#define SD_RESP_1b MMC_CMD_EXT_LATENCY | MMC_CMD_R1b

// R2 CID and CSD registers
// 136-bit response
// [127:1] CID or CSD including CRC
#define SD_RESP_2  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP136

// R3 OCR register
// 48-bit response
// [39:8] OCR register
// NO CRC
#define SD_RESP_3  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48

// R6 RCA response
// 48-bit
// [15:0] card status bits 23, 22, 19, 12:0
// [31:16] new published RCA
#define SD_RESP_6  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48

// R7 card interface condition
// 48-bit
// [19:16] supported voltage range typical 0b0001 2.7-3.6V
// [15:8] echo check pattern
#define SD_RESP_7  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48

/// This SD protocol inplementation only support one slot access
void sd_protocol_init(void);

void sd_write(u32 sector, u32 size, u8* buffer);

void sd_read(u32 sector, u32 size, u8* buffer);

void sd_dma_write(u32 sector, u32 size, u8* buffer);

void sd_dma_read(u32 sector, u32 size, u8* buffer);

#endif