/// Copyright (C) StrawberryHacker

#include "matrix.h"
#include "hardware.h"

void matrix_cs_init(enum cs1_select cs1, uint8_t assign_cs2_flash,
                    uint8_t assign_cs3_flash, uint8_t assign_cs0_flash) {

	uint32_t tmp_reg =	(0b1 & assign_cs0_flash) |
                        ((0b1 & assign_cs2_flash) << 2) |
	                    ((0b1 & assign_cs3_flash) << 3);
	
	if (cs1 == CS1_SDRAM) {
		tmp_reg |= (1 << 4);
	} else if (cs1 == CS1_NAND_FLASH) {
		tmp_reg |= 0b1;
	}
	
	MATRIX->SMCNFCS = tmp_reg;
}
