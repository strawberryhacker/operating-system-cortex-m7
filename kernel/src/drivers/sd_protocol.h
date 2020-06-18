#ifndef SD_PROTOCOL_H
#define SD_PROTOCOL_H

#include "types.h"

struct sd_card {
    u32 rca;
    u8  initialized : 1;
};

/// This SD protocol inplementation only support one slot access
void sd_protocol_init(void);

void sd_write(u32 sector, u32 size, u8* buffer);

void sd_read(u32 sector, u32 size, u8* buffer);

void sd_dma_write(u32 sector, u32 size, u8* buffer);

void sd_dma_read(u32 sector, u32 size, u8* buffer);

#endif