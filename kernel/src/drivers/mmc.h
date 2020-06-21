/// Copyright (C) StrawberryHacker

#ifndef MMC_H
#define MMC_H

#include "types.h"

enum mmc_bus_width {
    MMC_1_LANE,
    MMC_4_LANES = 2
};

enum mmc_dma_chunk {
    MMC_CHUNK_1,
    MMC_CHUNK_2,
    MMC_CHUNK_4,
    MMC_CHUNK_8,
    MMC_CHUNK_16
};

/// Some CMD register defines used in the SD protocol
#define MMC_CMD_READ        (1 << 18)
#define MMC_CMD_WRITE       (0 << 18)

#define MMC_CMD_SINGLE      (0 << 19)
#define MMC_CMD_MULTIPLE    (1 << 19)
#define MMC_CMD_STREAM      (2 << 19)
#define MMC_CMD_BYTE        (4 << 19)
#define MMC_CMD_BLOCK       (5 << 19)

#define MMC_CMD_NO_DATA     (0 << 16)
#define MMC_CMD_START_DATA  (1 << 16)
#define MMC_CMD_STOP_DATA   (2 << 16)

#define MMC_CMD_EXT_LATENCY (1 << 12)

#define MMC_CMD_OPEN_DRAIN  (1 << 11)

#define MMC_CMD_RESP48      (1 << 6)
#define MMC_CMD_RESP136     (2 << 6)
#define MMC_CMD_R1b         (3 << 6)

#define MMC_CMD_INIT        (1 << 8)
#define MMC_CMD_SYNC        (2 << 8)
#define MMC_CMD_CE_ATA      (3 << 8)
#define MMC_CMD_IT_CMD      (4 << 8)
#define MMC_CMD_IT_RESP     (5 << 8)
#define MMC_CMD_BOR         (6 << 8)
#define MMC_CMD_EBO         (7 << 8)


void mmc_init(void);

void mmc_reset(void);

void mmc_enable(void);

void mmc_disable(void);

void mmc_set_bus_freq(u32 frequency);

void mmc_set_bus_width(enum mmc_bus_width width);

void mmc_enable_high_speed(void);

void mmc_disable_high_speed(void);

void mmc_dma_enable(enum mmc_dma_chunk chunk);

void mmc_dma_disable(void);

u8 mmc_send_cmd(u32 cmd, u32 arg, u8 check_crc);

u8 mmc_send_adtc(u32 cmd, u32 arg, u32 block_size, u32 block_count, u8 check_crc);

u32 mmc_read_resp48(void);

void mmc_read_resp136(u8* buffer);

u32 mmc_read_data(void);

void mmc_read_data_reverse(u8* buffer, u32 word_count);

void mmc_write_data(u32 data);

#endif
