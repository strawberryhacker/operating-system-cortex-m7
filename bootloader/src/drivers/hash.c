/* Copyright (C) StrawberryHacker */

#include "hash.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"

/*
 * List descriptor is fetched from memory by the hash engine. The first
 * descriptor address must be aligned by 64 bytes according to the datasheet 
 * spesification page 1755
 */
struct icm_desc {
	/* Start address of the current block */
	u32 start_addr;

	/* Configures the operation applying to one region */
	u32 cfg;

	/* The size field must be set to the number of 512-bit blocks minus one */
	u32 size;

	/* 
	 * his point to the next descriptor in the secondary list. Must be zero
	 * if secondary list branching is disabled
	 */
	struct icm_desc* next;
};

/* Currently the hash engine only supports single region hash calculation */
__attribute__((aligned(64))) struct icm_desc region_one;

/*
 * Computes the SHA-256 over the `data` region with `size` number of bytes, and
 * writes the computed hash to the memory pointed to by `hash`. This address must be
 * 128-byte aligned
 */
void hash256_generate(const void* data, u32 size, u8* hash) {

	ICM->CTRL = (1 << 2) | (1 << 1) | (0b111 << 9);
	
	/*
	 * Write configuration register. SHA256, secondary list branch disable
	 * and custom initial hash
	 */
	ICM->CFG = (1 << 2) | (1 << 13);
	
	region_one.start_addr = (u32)data;
	region_one.cfg        = (1 << 12) | (1 << 2);
	region_one.size       = (size / 32) - 1;
	region_one.next       = 0;
	
	ICM->DSCR = (u32)&region_one;
	ICM->HASH = (u32)hash;

	/* Initialize the hash memory destination to zero */
	for (u8 i = 0; i < 32; i++) {
		hash[i] = 0x00;
	}

	ICM->CTRL = (1 << 0);

	u32 status;
	do {
		status = ICM->ISR;
	} while (!(status & 1));
	
	if (status & ((1 << 24) | (0b1111 << 8))) {
		print("Error: %32b\n", status);
		panic("Hash error");
	}

	ICM->CTRL = (1 << 2);
}

/*
 * Computes the SHA-256 over the `data` region with `size` number of bytes, and
 * compares the computed value agains the hash present at `hash`. It returns
 * `1` if these match, `0` if not
 */
u8 hash256_verify(const void* data, u32 size, const u8* hash) {

	ICM->CTRL = (1 << 2) | (1 << 1) | (0b111 << 9);
	
	/*
	 * Write configuration register. SHA256, secondary list branch disable
	 * and custom initial hash
	 */
	ICM->CFG = (1 << 2) | (1 << 13);
	
	region_one.start_addr = (u32)data;
	region_one.cfg        = (1 << 12) | (1 << 2) | (1 << 0);
	region_one.size       = (size / 32) - 1;
	region_one.next       = 0;
	
	ICM->DSCR = (u32)&region_one;
	ICM->HASH = (u32)hash;

	ICM->CTRL = (1 << 0);

	u32 status;
	do {
		status = ICM->ISR;
	} while (!(status & 1));
	
	if (status & ((1 << 24) | (0b1111 << 8))) {
		print("Error: %32b\n", status);
		panic("Hash error");
	}

	print("Status reg: %32b\n", status);

	ICM->CTRL = (1 << 2);

	return 0;
}
