/// Copyright (C) StrawberryHacker

#include "crc.h"

/// Calculates the CRC-8 of a data region using `polynomial`
u8 crc_calculate(const void* src, u32 size, u8 polynomial) {
    // Return value
    u8 crc = 0;

    const u8* src_ptr = (const u8 *)src;

    for (u32 i = 0; i < size; i++) {
        crc = crc ^ *src_ptr++;

        for (u8 j = 0; j < 8; j++) {
            if (crc & 0x01) {
                crc = crc ^ polynomial;
            }
            crc = crc >> 1;
        }
    }
    return crc;
}
