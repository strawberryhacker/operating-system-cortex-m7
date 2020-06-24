/// Copyright (C) StrawberryHacker

#ifndef STD_H
#define STD_H

#include "types.h"

void memory_copy(const void* src, void* dest, u32 size);

u8 memory_compare(const void* src1, const void* src2, u32 size);

void memory_fill(void* dest, u8 fill, u32 size);

u32 string_len(const char* src);

#endif
