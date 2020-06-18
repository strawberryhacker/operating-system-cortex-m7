#ifndef CACHE_H
#define CACHE_H

#include "types.h"

void dcache_enable(void);

void dcache_disable(void);

void dcache_invalidate(void);

void dcache_clean(void);

void dcache_invalidate_addr(const u32* addr, u32 size);

void dcache_clean_addr(const u32* addr, u32 size);

void icache_enable(void);

void icache_disable(void);

void icache_invalidate(void);

void icache_clean(void);

#endif