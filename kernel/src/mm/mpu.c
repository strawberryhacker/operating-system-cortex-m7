/* Copyright (C) StrawberryHacker */

#include "mpu.h"
#include "hardware.h"
#include "panic.h"
#include "cpu.h"

/*
 * Enables privileged software to access the default system memory map
 */
void mpu_enable_priv_access(void) {
    MPU->CTRL |= (1 << 2);
}

/*
 * Disables privileged software to access the default system memory map.
 * Any memory access to a non-covered region will cause a mem fault
 */
void mpu_disable_priv_access(void) {
    MPU->CTRL &= ~(1 << 2);
}

/*
 * Enables the MPU during hard fault, NMI and faultmask handlers
 */
void mpu_hard_fault_enable(void) {
    MPU->CTRL |= (1 << 1);
}

/*
 * Disables the MPU during hard fault, NMI and faultmask handlers
 */
void mpu_hard_fault_disable(void) {
    MPU->CTRL &= ~(1 << 1);
}

/*
 * Enables the MPU
 */
void mpu_enable(void) {
    MPU->CTRL |= (1 << 0);
    dsb();
    isb();
}

/*
 * Disables the MPU
 */
void mpu_disable(void) {
    MPU->CTRL &= ~(1 << 0);
}

/*
 * Returns the number of supported MPU data regions in the SoC
 */
u8 mpu_get_data_regions(void) {
    u32 reg = MPU->TYPE;
    return (reg >> 8) & 0xFF;
}

/*
 * Sets the region to configure
 */
static void mpu_set_region(u8 reg_num) {

    /* Check if the region number is supported */
    u8 max_region = mpu_get_data_regions();

    if (reg_num >= max_region) {
        panic("MPU region failure");
    }
    MPU->RNR = reg_num;
}

/*
 * Configures a MPU memory region
 */
void mpu_configure_region(u8 reg_num, u32 addr, struct mpu_region* reg_desc) {

    /* Set the region to configure */
    mpu_set_region(reg_num);

    /* Check input parameters */
    if (reg_desc->size < 4) {
        panic("Region size error");
    }

    /* Check if the address provided is valid */
    u32 addr_mask = (1 << (reg_desc->size + 1)) - 1;

    if (addr & addr_mask) {
        panic("Address is wrong");
    }

    /* Save configuration */
    MPU->RBAR = addr;

    u32 rasr = 0;
    rasr |= (reg_desc->ap << 24);
    rasr |= (reg_desc->tex << 19);
    rasr |= (reg_desc->c << 17);
    rasr |= (reg_desc->b << 16);
    rasr |= (reg_desc->s << 18);

    /* Execute never */
    if (reg_desc->executable == 0) {
        rasr |= (1 << 28);
    }

    /* Subregion configuration */
    rasr |= (reg_desc->subregion_mask << 8);

    /* Enable region and all subregions */
    rasr |= (reg_desc->enable << 0);

    /* Set the size */
    rasr |= (reg_desc->size << 1);

    dsb();
    isb();
    MPU->RASR = rasr;
}
