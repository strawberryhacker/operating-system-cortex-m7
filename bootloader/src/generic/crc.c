#include "crc.h"

const u8 crc_table[256] = {0};

u8 crc_calculate(const void* src, u32 size) {

    const u8* src_ptr = (const u8 *)src;
    for (u32 i = 0; i < size; i++) {
        
        src_ptr++;
    }
    return 0;
}