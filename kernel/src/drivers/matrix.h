/// Copyright (C) StrawberryHacker

#ifndef MATRIX_H
#define MATRIX_H

#include "types.h"

enum cs1_select {
	CS1_SDRAM,
	CS1_NAND_FLASH,
	CS1_NONE
};

void matrix_cs_init(enum cs1_select cs1, uint8_t assign_cs2_flash,
                    uint8_t assign_cs3_flash, uint8_t assign_cs0_flash);

#endif
