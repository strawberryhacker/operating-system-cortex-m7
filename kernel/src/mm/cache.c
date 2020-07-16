/* Copyright (C) StrawberryHacker */

#include "cache.h"
#include "hardware.h"
#include "print.h"
#include "cpu.h"
#include "panic.h"

/*
 * Returns information abount the cache unit
 */
void cache_info(void) {
    print("Cache: %32b\n", SCB->CCSIDR);
}

/*
 * Enables and invalidates the D-cache
 */
void dcache_enable(void) {
    /* Select the L1 data cache */
    SCB->CSSELR = 0;
    dsb();

    /*
     * Read the cache size ID register to determite the number
     * of ways and number of sets
     */
    u32 cache_id = SCB->CCSIDR;

    /* Invalidate the data cache by set and way; one line at a time */
    u32 num_sets = ((cache_id >> 13) & 0x7FFF) + 1;
    do {
        u32 num_ways = ((cache_id >> 3) & 0x3FF) + 1;
        do {
            SCB->DCISW = (num_sets << 5) | (num_ways << 30);
            isb();
        } while (num_ways--);
    } while (num_sets--);
    dsb();

    /* Enable the D-cache */
    SCB->CCR |= (1 << 16);
    dsb();
    isb();
}

/*
 * Disables the D-cache and cleans and invalidated all data
 */
void dcache_disable(void) {
    /* Select the L1 data cache */
    SCB->CSSELR = 0;
    dsb();

    /* Disable the D-cache */
    SCB->CCR &= ~(1 << 16);
    dsb();
    isb();

    /*
     * Read the cache size ID register to determite the number
     * of ways and number of sets
     */
    u32 cache_id = SCB->CCSIDR;

    /* Invalidate the data cache by set and way; one line at a time */
    u32 num_sets = ((cache_id >> 13) & 0x7FFF) + 1;
    do {
        u32 num_ways = ((cache_id >> 3) & 0x3FF) + 1;
        do {
            SCB->DCCISW = (num_sets << 5) | (num_ways << 30);
            isb();
        } while (num_ways--);
    } while (num_sets--);
    
    dsb();
    isb();
}

/*
 * Invalidates the entire D-cache
 */
void dcache_invalidate(void) {
    /* Select the L1 data cache */
    SCB->CSSELR = 0;
    dsb();

    /*
     * Read the cache size ID register to determite the number
     * of ways and number of sets
     */
    u32 cache_id = SCB->CCSIDR;

    /* Invalidate the data cache by set and way; one line at a time */
    u32 num_sets = ((cache_id >> 13) & 0x7FFF) + 1;
    do {
        u32 num_ways = ((cache_id >> 3) & 0x3FF) + 1;
        do {
            SCB->DCISW = (num_sets << 5) | (num_ways << 30);
            isb();
        } while (num_ways--);
    } while (num_sets--);
    
    dsb();
    isb();
}

/*
 * Cleans the entire D-cache
 */
void dcache_clean(void) {
    /* Select the L1 data cache */
    SCB->CSSELR = 0;
    dsb();

    /*
     * Read the cache size ID register to determite the number
     * of ways and number of sets
     */
    u32 cache_id = SCB->CCSIDR;

    /* Invalidate the data cache by set and way; one line at a time */
    u32 num_sets = ((cache_id >> 13) & 0x7FFF) + 1;
    do {
        u32 num_ways = ((cache_id >> 3) & 0x3FF) + 1;
        do {
            SCB->DCCSW = (num_sets << 5) | (num_ways << 30);
            isb();
        } while (num_ways--);
    } while (num_sets--);
    
    dsb();
    isb();
}

/*
 * Cleans and invalidated the D-cache
 */
void dcache_clean_invalidate(void) {
    /* Select the L1 data cache */
    SCB->CSSELR = 0;
    dsb();

    /*
     * Read the cache size ID register to determite the number
     * of ways and number of sets
     */
    u32 cache_id = SCB->CCSIDR;

    /* Invalidate the data cache by set and way; one line at a time */
    u32 num_sets = ((cache_id >> 13) & 0x7FFF) + 1;
    do {
        u32 num_ways = ((cache_id >> 3) & 0x3FF) + 1;
        do {
            SCB->DCCISW = (num_sets << 5) | (num_ways << 30);
            isb();
        } while (num_ways--);
    } while (num_sets--);
    
    dsb();
    isb();
}

/*
 * Invalidated the D-cache by virtual address to PoC. The input
 * address must be aligned with 32 bytes. 
 */
void dcache_invalidate_addr(const u32* addr, u32 size) {
    dsb();

    /* Check input alignment */
    if ((u32)addr & 0x1F) {
        panic("Unaligned cache address detected");
    }

    const u8* addr_ptr = (const u8 *)addr;

    /* Cache invalidation by 32 byte block */
    for (u32 i = 0; i < size; i += 32) {
        SCB->DCIMVAC = (u32)addr_ptr;
        addr_ptr += 32;
    }

    dsb();
    isb();
}

/*
 * Cleans the D-cache by virtual address to PoC. The input
 * address must be aligned with 32 bytes. 
 */
void dcache_clean_addr(const u32* addr, u32 size) {
    dsb();

    /* Check input alignment */
    if ((u32)addr & 0x1F) {
        panic("Unaligned cache address detected");
    }

    const u8* addr_ptr = (const u8 *)addr;

    /* Cache invalidation by 32 byte block */
    for (u32 i = 0; i < size; i += 32) {
        SCB->DCCMVAC = (u32)addr_ptr;
        addr_ptr += 32;
    }

    dsb();
    isb();    
}

/*
 * Cleans and invalidates the D-cache by virtual address to PoC.\
 * The input address must be aligned with 32 bytes. 
 */
void dcache_clean_invalidate_addr(const u32* addr, u32 size) {
    dsb();

    /* Check input alignment */
    if ((u32)addr & 0x1F) {
        panic("Unaligned cache address detected");
    }

    const u8* addr_ptr = (const u8 *)addr;

    /* Cache invalidation by 32 byte block */
    for (u32 i = 0; i < size; i += 32) {
        SCB->DCCIMVAC = (u32)addr_ptr;
        addr_ptr += 32;
    }

    dsb();
    isb(); 
}

/*
 * Enables I-cache
 */
void icache_enable(void) {
    /* Invalidate the I-cache */
    dsb();
    isb();
    SCB->ICIALLU = 0;

    /* Enable I-cache */
    dsb();
    isb();
    SCB->CCR |= (1 << 17);

    dsb();
    isb();
}

/*
 * Disables I-cache
 */
void icache_disable(void) {
    /* Disable I-cache */
    dsb();
    isb();
    SCB->CCR &= ~(1 << 17);
    SCB->ICIALLU = 0;
    dsb();
    isb();
}

/*
 * Invalidates the I-cache
 */
void icache_invalidate(void) {
    dsb();
    isb();
    SCB->ICIALLU = 0;
    dsb();
    isb();
}
