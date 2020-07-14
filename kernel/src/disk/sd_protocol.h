/* Copyright (C) StrawberryHacker */

#ifndef SD_PROTOCOL_H
#define SD_PROTOCOL_H

#include "types.h"
#include "mmc.h"

#define SD_DEBUG 0

struct sd_card {
    u32  rca;
    char cid_name[6];
    u8   initialized : 1;
    u8   high_capacity : 1;
    u8   four_bit_bus_support : 1;
    u8   past_v1_10 :1;
    u8   high_speed : 1;

    /* Size in KiB */
    u32  kib_size; 
    u32  block_count;
};

/* Defines the different SD card responses */
#define SD_RESP_1  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48
#define SD_RESP_1b MMC_CMD_EXT_LATENCY | MMC_CMD_R1b
#define SD_RESP_2  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP136
#define SD_RESP_3  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48
#define SD_RESP_6  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48
#define SD_RESP_7  MMC_CMD_EXT_LATENCY | MMC_CMD_RESP48

/* This SD protocol inplementation only support one slot access */
void sd_protocol_init(void);

u8 sd_write(u32 sector, u32 count, const u8* buffer);

u8 sd_read(u32 sector, u32 count, u8* buffer);

u8 sd_dma_write(u32 sector, u32 count, const u8* buffer);

u8 sd_dma_read(u32 sector, u32 count, u8* buffer);

#endif
