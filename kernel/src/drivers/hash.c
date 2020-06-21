/// Copyright (C) StrawberryHacker

#include "hash.h"
#include "hardware.h"
#include "panic.h"

/// This descriptor describes ICM block number `1`. It must be aligned by 
/// 512-bit
__attribute__((__aligned__(64))) struct icm_desc hash_descriptor;

enum icm_algorithm {
	SHA1,
	SHA256,
	SHA224
};

struct icm_s {
	// Sets the SHA algorithm
	enum icm_algorithm algorithm;
	
	// 4-bit setting the delay cylces between bursts
	u8 bus_utilization : 4;
	
	u8 custom_initial_hash : 1;
	u8 dual_input_buffer : 1;
	u8 auto_monitor_mode : 1;
	u8 end_monitor_disable : 1;
	u8 write_back_disable : 1;
	u8 list_branch_disable : 1;
};

struct icm_desc {
	// Start address of the current block
	u32 start_addr;
	u32 cfg;
	u32 size;
	struct icm_desc* next;
};

void hash256_generate(const void* data, u32 size, u8* hash) {
	ICM->CTRL = (1 << 2);
	
	// Write configuration register. SHA256, secondary list branch disable
	// and custom initial hash
	ICM->CFG = (1 << 2) | (1 << 13);
	
	hash_descriptor.start_addr = (u32)data;
	hash_descriptor.cfg = (1 << 12) | (1 << 2) | (0 << 0);
	hash_descriptor.size = (size / 32) - 1;
	hash_descriptor.next = 0;
	
	ICM->DSCR = (u32)&hash_descriptor;
	ICM->HASH = (u32)hash;

	ICM->CTRL = (1 << 0);
	
	u32 status;
	do {
		status = ICM->ISR;
	} while (!(status & 1));
	
	if (status & ((1 << 24) | (0b1111 << 8))) {
		panic("Hash error");
	}
	
	ICM->CTRL = (1 << 2) | (1 << 1);
	
}
