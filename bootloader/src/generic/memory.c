/* Copyright (C) StrawberryHacker */

#include "memory.h"

/*
 * Copies `size` bytes from `src` to `dest`
 */
void memory_copy(const void* src, void* dest, u32 size) {
    const u8* src_ptr = (const u8 *)src;
    u8* dest_ptr = (u8 *)dest;

    while (size--) {
        *dest_ptr++ = *src_ptr++;
    }
}

/*
 * Compares the memory content in the to locations `src1` and `src2` and 
 * returns `1` if the memory regions match
 */
u8 memory_compare(const void* src1, const void* src2, u32 size) {
    const u8* src1_ptr = (const u8 *)src1;
    const u8* src2_ptr = (const u8 *)src2;

    while (size--) {
        if (*src1_ptr != *src2_ptr) {
            return 0;
        }
        src1_ptr++;
        src2_ptr++;
    }
    return 1;
}

/*
 * Fills a memory region with the value `fill`
 */
void memory_fill(void* dest, u8 fill, u32 size) {
    u8* dest_ptr = (u8 *)dest;

    while (size--) {
        *dest_ptr++ = fill;
    }
}

/*
 * Takes in a pointer to a char array and returns the size of it. It must end
 * with a zero terminator - if not the result is undefined
 */
u32 string_len(const char* src) {
    u32 ret = 0;

    while (*src++) {
        ret++;
    }
    return ret;
}
