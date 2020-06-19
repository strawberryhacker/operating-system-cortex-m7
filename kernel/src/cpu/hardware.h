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

typedef struct {
	_rw u32 CFG;
	_w  u32 CTRL;
	_r  u32 SR;
	_r  u32 RESERVED1;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_r  u32 ISR;
	_r  u32 UASR;
	_r  u32 RESERVED2[3];
	_rw u32 DSCR;
	_rw u32 HASH;
	_w  u32 UIHVAL[8];
} icm_reg;

typedef struct {
	_rw u32 PRAS;
	_rw u32 PRBS;
} matrix_pri_reg;

typedef struct {
	_rw u32 MCFG[13];
	_r  u32 RESERVED1[3];
	_rw u32 SCFG[9];
	_r  u32 RESERVED2[7];
	matrix_pri_reg PRI[9];
	_r  u32 RESERVED3[14];
	_rw u32 MRCR;
	_rw u32 RESERVED4[3];
	_rw u32 CAN0;
	_rw u32 SYSIO;
	_rw u32 PCCR;
	_rw u32 DYNCKG;
	_r  u32 RESERVED5;
	_rw u32 SMCNFCS;
	_r  u32 RESERVED6[47];
	_rw u32 WPMR;
	_r  u32 WPSR;
} matrix_reg;

typedef struct {
	_rw u32 MR;
	_rw u32 TR;
	_rw u32 CR;
	_r  u32 RESERVED;
	_rw u32 LPR;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_r  u32 ISR;
	_rw u32 MDR;
	_rw u32 CFR1;
	_rw u32 OCMS;
	_w  u32 OCMS_KEY1;
	_w  u32 OCMS_KEY2;
} dram_reg;

typedef struct {
	_r  u32 CPUID;
	_rw u32 ICSR;
	_rw u32 VTOR;
	_rw u32 AIRCR;
	_rw u32 SCR;
	_rw u32 CCR;
	_rw u8  SHPR[12];
	_rw u32 SHCSR;
	_rw u32 CFSR;
	_rw u32 HFSR;
	_rw u32 DFSR;
	_rw u32 MMFAR;
	_rw u32 BFAR;
	_rw u32 AFSR;
	_r  u32 ID_PFR[2];
	_r  u32 ID_DFR;
	_r  u32 ID_AFR;
	_r  u32 ID_MFR[4];
	_r  u32 ID_ISAR[5];
    _r  u32 RESERVED0[1];
	_r  u32 CLIDR;
	_r  u32 CTR;
	_r  u32 CCSIDR;
	_rw u32 CSSELR;
	_rw u32 CPACR;
    _r  u32 RESERVED3[93];
	_w  u32 STIR;
    _r  u32 RESERVED4[15];
	_r  u32 MVFR0;
	_r  u32 MVFR1;
	_r  u32 MVFR2;
	_r  u32 RESERVED5[1];
	_w  u32 ICIALLU;
    _r  u32 RESERVED6[1];
	_w  u32 ICIMVAU;
	_w  u32 DCIMVAC;
	_w  u32 DCISW;
	_w  u32 DCCMVAU;
	_w  u32 DCCMVAC;
	_w  u32 DCCSW;
	_w  u32 DCCIMVAC;
	_w  u32 DCCISW;
    _r  u32 RESERVED7[6];
	_rw u32 ITCMCR;
	_rw u32 DTCMCR;
	_rw u32 AHBPCR;
	_rw u32 CACR;
	_rw u32 AHBSCR;
    _r  u32 RESERVED8[1];
	_rw u32 ABFSR;
} scb_reg;

typedef struct {
	_w  u32 ICIALLU;
	_r  u32 RESERVED;
	_w  u32 ICIMVAU;
	_w  u32 DCIMVAU;
	_w  u32 DCISW;
	_w  u32 DCCMVAU;
	_w  u32 DCCMVAC;
	_w  u32 DCCSW;
	_w  u32 DCCIMVAC;
	_w  u32 DCCISW;
	_r  u32 DONT_USE;
} cache_reg;

typedef struct {
	_w  u32 CR;
	_rw u32 MR;
	_rw u32 DTOR;
	_rw u32 SDCR;
	_rw u32 ARG;
	_w  u32 CMD;
	_rw u32 BLKR;
	_rw u32 CSTOR;
	_r  u32 RESP[4];
	_r  u32 RDR;
	_w  u32 TDR;
	_r  u32 RESERVED1[2];
	_r  u32 SR;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_rw u32 DMA;
	_rw u32 CFG;
	_r  u32 RESERVED2[35];
	_rw u32 WPMR;
	_r  u32 WPSR;
	_r  u32 RESERVED3[69];
	_rw u32 FIFO[256];
} mmc_reg;

#define CLOCK ((clock_reg *)0x400E0600)
#define FLASH ((flash_reg *)0x400E0C00)
#define WATCHDOG ((watchdog_reg *)0x400E1850)
#define SYSTICK ((systick_reg *)0xE000E010)
#define NVIC ((nvic_reg *)0xE000E100)
#define ICM ((icm_reg *)0x40048000)
#define MATRIX ((matrix_reg *)0x40088000)
#define DRAM ((dram_reg *)0x40084000)
#define SCB ((scb_reg *)0xE000ED00)
#define CACHE ((scb_reg *)0xE000EF50)
#define MMC ((mmc_reg *)0x40000000)

#define GPIOA ((gpio_reg *)0x400E0E00)
#define GPIOB ((gpio_reg *)0x400E1000)
#define GPIOC ((gpio_reg *)0x400E1200)
#define GPIOD ((gpio_reg *)0x400E1400)
#define GPIOE ((gpio_reg *)0x400E1600)

#define USART0 ((usart_reg *)0x40024000)
#define USART1 ((usart_reg *)0x40028000)
#define USART2 ((usart_reg *)0x4002C000)

#define VECTOR_TABLE_BASE 0xE000ED08

#define KERNEL_IMAGE_ADDR 0x00404200


#endif
