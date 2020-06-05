/// Copyright (C) StrawberryHacker

#ifndef HARDWARE_H
#define HARDWARE_H

#include "types.h"

#define _r volatile const
#define _w volatile 
#define _rw volatile

/// Clock registers
typedef struct {
	_w  u32 SCER;
	_w  u32 SCDR;
	_r  u32 SCSR;
	_r  u32 RESERVED_1;
	_w  u32 PCER0;
	_w  u32 PCDR0;
	_r  u32 PCSR0;
	_rw u32 UCKR;
	_rw u32 MOR;
	_rw u32 MCFR;
	_rw u32 PLLA;
	_r  u32 RESERVED_2;
	_rw u32 MCKR;
	_r  u32 RESERVED_3;
	_rw u32 USB;
	_r  u32 RESERVED_4;
	_rw u32 PCK[8];
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 SR;
	_r  u32 IMR;
	_rw u32 FSMR;
	_rw u32 FSPR;
	_w  u32 FOCR;
	_r  u32 RESERVED_5[26];
	_rw u32 WPMR;
	_r  u32 WPSR;
	_r  u32 RESERVED_6[5];
	_w  u32 PCER1;
	_w  u32 PCDR1;
	_r  u32 PCSR1;
	_rw u32 PCR;
	_rw u32 OCR;
	_w  u32 SLPWK_ER0;
	_w  u32 SLPWK_DR0;
	_r  u32 SLPWK_SR0;
	_r  u32 SLPWK_ASR0;
	_r  u32 RESERVED_7[3];
	_rw u32 PMMR;
	_w  u32 SLPWK_ER1;
	_w  u32 SLPWK_DR1;
	_r  u32 SLPWK_SR1;
	_r  u32 SLPWK_ASR1;
	_r  u32 AIPR;
} clock_reg;

/// Flash registers
typedef struct {
	_rw u32 FMR;
	_w  u32 FCR;
	_r  u32 FSR;
	_r  u32 FRR;
	_r  u32 RESERVED[53];
	_rw u32 WPMR;
} flash_reg;

/// Watchdog registers
typedef struct {
	_w  u32 CR;
	_rw u32 MR;
	_r  u32 SR;
} watchdog_reg;

/// GPIO registers
typedef struct {
	_w  u32 PER;
	_w  u32 PDR;
	_r  u32 PSR;
	_r  u32 RESERVED_1;
	_w  u32 OER;
	_w  u32 ODR;
	_r  u32 OSR;
	_r  u32 RESERVED_2;
	_w  u32 IFER;
	_w  u32 IFDR;
	_r  u32 IFSR;
	_r  u32 RESERVED_3;
	_w  u32 SODR;
	_w  u32 CODR;
	_r  u32 ODSR;
	_r  u32 PDSR;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_r  u32 ISR;
	_w  u32 MDER;
	_w  u32 MDDR;
	_r  u32 MDSR;
	_r  u32 RESERVED_4;
	_w  u32 PUDR;
	_w  u32 PUER;
	_r  u32 PUSR;
	_r  u32 RESERVED_5;
	_rw u32 ABCDSR0;
	_rw u32 ABCDSR1;
	_r  u32 RESERVED_6[2];
	_w  u32 IFSCDR;
	_w  u32 IFSCER;
	_r  u32 IFSCSR;
	_rw u32 SCDR;
	_w  u32 PPDDR;
	_w  u32 PPDER;
	_r  u32 PPDSR;
	_r  u32 RESERVED_7;
	_w  u32 OWER;
	_w  u32 OWDR;
	_r  u32 OWSR;
	_r  u32 RESERVED_8;
	_w  u32 AIMER;
	_w  u32 AIMDR;
	_r  u32 AIMMR;
	_r  u32 RESERVED_9;
	_w  u32 ESR;
	_w  u32 LSR;
	_r  u32 ELSR;
	_r  u32 RESERVED_10;
	_w  u32 FELLSR;
	_w  u32 REHLSR;
	_r  u32 FRLHSR;
	_r  u32 RESERVED_11;
	_r  u32 LOCKSR;
	_rw u32 WPMR;
	_r  u32 WPSR;
	_r  u32 RESERVED_12[5];
	_rw u32 SCHMITT;
	_r  u32 RESERVED_13[5];
	_rw u32 DRIVER;
	_r  u32 RESERVED_14[13];
	_rw u32 PCMR;
	_w  u32 PCIER;
	_w  u32 PCIDR;
	_r  u32 PCIMR;
	_r  u32 PCISR;
	_r  u32 PCRHR;
} gpio_reg;

/// Systick registers
typedef struct {
	_rw u32 CSR;
	_rw u32 RVR;
	_rw u32 CVR;
	_r  u32 CALIB;
} systick_reg;

/// USART registers
typedef struct {
	_w  u32 CR;
	_rw u32 MR;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_r  u32 CSR;
	_r  u32 RHR;
	_w  u32 THR;
	_rw u32 BRGR;
	_rw u32 RTOR;
	_rw u32 TTGR;
	_r  u32 RESERVED1[4];
	_rw u32 FTDI;
	_r  u32 NER;
} usart_reg;

/// NVIC registers
typedef struct {
	_rw u32 ISER[8];
	_r  u32 RESERVED1[24];
	_rw u32 ICER[8];
	_r  u32 RESERVED2[24];
	_rw u32 ISPR[8];
	_r  u32 RESERVED3[24];
	_rw u32 ICPR[8];
	_r  u32 RESERVED4[24];
	_rw u32 IABR[8];
	_r  u32 RESERVED5[56];
	_rw u8  IPR[240];
	_r  u32 RESERVED6[644];
	_r  u32 STIR;
} nvic_reg;

#define CLOCK ((clock_reg *)0x400E0600)
#define FLASH ((flash_reg *)0x400E0C00)
#define WATCHDOG ((watchdog_reg *)0x400E1850)
#define SYSTICK ((systick_reg *)0xE000E010)
#define NVIC ((nvic_reg *)0xE000E100)

#define GPIOA ((gpio_reg *)0x400E0E00)
#define GPIOB ((gpio_reg *)0x400E1000)
#define GPIOC ((gpio_reg *)0x400E1200)
#define GPIOD ((gpio_reg *)0x400E1400)
#define GPIOE ((gpio_reg *)0x400E1600)

#define USART0 ((usart_reg *)0x40024000)
#define USART1 ((usart_reg *)0x40028000)
#define USART2 ((usart_reg *)0x4002C000)

#endif