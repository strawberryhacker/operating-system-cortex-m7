#include "memory.h"

void memory_copy(const void* src, void* dest, u32 size) {
    const u8* src_ptr = (const u8 *)src;
    u8* dest_ptr = (u8 *)dest;

    while (size--) {
        *dest_ptr++ = *src_ptr++;
    }
}

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

void memory_fill(void* dest, u8 fill, u32 size) {
    u8* dest_ptr = (u8 *)dest;

    while (size--) {
        *dest_ptr++ = fill;
    }
}