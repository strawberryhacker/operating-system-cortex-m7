/// Copyright (C) StrawberryHacker

#include "dram.h"
#include "gpio.h"
#include "clock.h"
#include "matrix.h"

enum dram_mode {
	DRAM_MODE_NORMAL,
	DRAM_MODE_NOP,
	DRAM_MODE_PRECHARGE,
	DRAM_MODE_LOAD_MR,
	DRAM_MODE_REFRESH,
	DRAM_MODE_LOAD_EXT_MR,
	DRAM_MODE_POWERDOWN
};

static inline void dram_set_mode(enum dram_mode mode) {
	DRAM->MR = mode;
}

void dram_init(void) {
    // Enable SDRAM clock
    peripheral_clock_enable(62);

    // Initialize all SDRAM pins
    gpio_set_function(GPIOC, 20, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 21, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 22, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 23, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 24, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 25, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 26, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 27, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 28, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 29, GPIO_FUNC_A);
	
	gpio_set_function(GPIOA, 20, GPIO_FUNC_C);
	gpio_set_function(GPIOD, 17, GPIO_FUNC_C);
	gpio_set_function(GPIOC, 0, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 1, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 2, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 3, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 4, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 5, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 6, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 7, GPIO_FUNC_A);
	
	gpio_set_function(GPIOE, 0, GPIO_FUNC_A);
	gpio_set_function(GPIOE, 1, GPIO_FUNC_A);
	gpio_set_function(GPIOE, 2, GPIO_FUNC_A);
	gpio_set_function(GPIOE, 3, GPIO_FUNC_A);
	gpio_set_function(GPIOE, 4, GPIO_FUNC_A);
	gpio_set_function(GPIOE, 5, GPIO_FUNC_A);
	
	gpio_set_function(GPIOA, 15, GPIO_FUNC_A);
	gpio_set_function(GPIOA, 16, GPIO_FUNC_A);
	gpio_set_function(GPIOC, 18, GPIO_FUNC_A);
	
	gpio_set_function(GPIOD, 15, GPIO_FUNC_C);
	gpio_set_function(GPIOD, 16, GPIO_FUNC_C);
	
	gpio_set_function(GPIOD, 13, GPIO_FUNC_C);
	gpio_set_function(GPIOD, 23, GPIO_FUNC_C);
	gpio_set_function(GPIOD, 14, GPIO_FUNC_C);
	gpio_set_function(GPIOC, 15, GPIO_FUNC_A);
	gpio_set_function(GPIOD, 29, GPIO_FUNC_C);

    volatile u16* dram_addr = (volatile u16 *)0x70000000;
    volatile uint32_t i;
	
	// Start of initialization process. Refer to data sheet
	// Set chip select 1 to SDRAM in the bus MATRIX
	matrix_cs_init(CS1_SDRAM, 0, 0, 0);
	
	// Step 1
	// Write CR and CFR1 register
	DRAM->CR   = 0xF955D5E0;
	DRAM->CFR1 = 0x00000102;
	
	// Step 2
	// Configure low power register
	DRAM->LPR = 0x00;
	
	// Step 3
	// Select SDRAM memory type
	DRAM->MDR = 0;
	
	// Step 4
	// Pause of 200 us
	for (i = 0; i < ((150000000 / 1000000) * 200 / 6); i++);
	
	// Step 5
	// Issue a NOP command and perform any write operation
	dram_set_mode(DRAM_MODE_NOP);
	*dram_addr = 0x00;
	
	// Step 6
	// Charge all banks
	dram_set_mode(DRAM_MODE_PRECHARGE);
	*dram_addr = 0x00;
	for (i = 0; i < ((150000000 / 1000000) * 200 / 6); i++);
	
	// Step 7
	// Perform eight auto refresh
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x01;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x02;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x03;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x04;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x05;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x06;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x07;
	dram_set_mode(DRAM_MODE_REFRESH);
	*dram_addr = 0x08;
	
	// Step 8
	dram_set_mode(DRAM_MODE_LOAD_MR);
	*((uint16_t *)(dram_addr + 0x30)) = 0xfefa;
	for (i = 0; i < ((150000000 / 1000000) * 200 / 6); i++);

	
	//step 10
	//go into normal mode
	dram_set_mode(DRAM_MODE_NORMAL);
	*dram_addr = 0x00;
	
	//step 11
	i = 150000000 / 1000;
	i *= 15625;
	i /= 1000000;
	DRAM->TR = i & 0xFFF;
}