/* Copyright (C) StrawberryHacker */

#ifndef MPU_H
#define MPU_H

#include "types.h"

/*
 *    AP       Privileged    Unprivileged
 *  0b001      RW            No access
 *  0b010      RW            RO
 *  0b011      RW            RW
 *  0b101      RO            No access
 *  0b110      RO            RO
 */

/*
 *   TEX   C  B  S   Description
 *  0b000  0  0  x   Strongly ordered
 *  0b000  0  1  x   Shared device
 *  0b000  1  0  0   Write through, no write alloc,       non-sharable
 *  0b000  1  0  1   Write through, no write alloc,       sharable
 *  0b000  1  1  0   Write back,    no write alloc,       non-sharable
 *  0b000  1  1  1   Write back,    no write alloc,       sharable
 *  0b001  1  1  0   Write back,    write and read alloc, non-sharable
 *  0b001  1  1  1   Write back,    write and read alloc, sharable
 *  0b001  0  0  0   Non-cacheable,                       non-sharable
 *  0b001  0  0  1   Non-cacheable,                       sharable
 *  0b010  0  1  x   Non-sharable device
 */

struct mpu_region {
    /*
     * Determines the size of a memory region. The size is computed
     * as 2 in the power of (size + 1). The minumum value is 0b100
     */
    u8 size;

    /* Please refer to the above table */
    u8 ap  : 3;

    /* Please refer to the above table */
    u8 tex : 3;
    u8 c   : 1;
    u8 b   : 1;
    u8 s   : 1;

    /* Write to 1 if the region should be executable */
    u8 executable : 1;

    /* Enable the region */
    u8 enable : 1;

    /*
     * Regions bigger than 128 B are split into 8 subregions. Every bit
     * corresponds to one subregion, the LSB being the subregion with
     * lowest memory address. Write one to disable the subregion. If 
     * the regions is smaller than 128 B write this to zero.
     */
    u8 subregion_mask; 
};

u8 mpu_get_data_regions(void);

void mpu_configure_region(u8 reg_num, u32 addr, struct mpu_region* reg_desc);

void mpu_enable(void);

void mpu_disable(void);

void mpu_hard_fault_enable(void);

void mpu_hard_fault_disable(void);

void mpu_enable_priv_access(void);

void mpu_disable_priv_access(void);

#endif
