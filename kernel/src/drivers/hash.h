/// Copyright (C) StrawberryHacker

#ifndef ICM_H
#define ICM_H

#include "types.h"

void hash256_generate(const void* data, u32 size, u8* hash);

u8 hash256_check(const void* data, u32 size, const u8* hash);

#endif