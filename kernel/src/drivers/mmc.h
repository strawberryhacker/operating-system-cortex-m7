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

void mmc_init(void);

void mmc_reset(void);

void mmc_enable(void);

void mmc_disable(void);

void mmc_set_bus_freq(u32 frequency);

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
