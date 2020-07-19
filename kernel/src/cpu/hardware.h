/* Copyright (C) StrawberryHacker */

#ifndef HARDWARE_H
#define HARDWARE_H

#include "types.h"

#define _r volatile const
#define _w volatile 
#define _rw volatile

/*
 * Clock registers
 */
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

/*
 * Flash registers
 */
typedef struct {
	_rw u32 FMR;
	_w  u32 FCR;
	_r  u32 FSR;
	_r  u32 FRR;
	_r  u32 RESERVED[53];
	_rw u32 WPMR;
} flash_reg;

/*
 * Watchdog registers
 */
typedef struct {
	_w  u32 CR;
	_rw u32 MR;
	_r  u32 SR;
} watchdog_reg;

/*
 * GPIO registers
 */
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

/*
 * Systick registers
 */
typedef struct {
	_rw u32 CSR;
	_rw u32 RVR;
	_rw u32 CVR;
	_r  u32 CALIB;
} systick_reg;

/*
 * USART registers
 */
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

/*
 * NVIC registers
 */
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

/*
 * Integrity check monitor registers
 */
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

/*
 * Bus matrix priority registers
 */
typedef struct {
	_rw u32 PRAS;
	_rw u32 PRBS;
} matrix_pri_reg;

/*
 * Bus matrix registers
 */
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

/*
 * DRAM controller registers
 */
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

/*
 * Cortex-M7 system control block registers
 */
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

/*
 * Cache registers
 */
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

/*
 * Memory proteciton unit
 */
typedef struct {
	_r  u32 TYPE;
	_rw u32 CTRL;
	_rw u32 RNR;
	_rw u32 RBAR;
	_rw u32 RASR;
	_rw u32 RBAR_A1;
	_rw u32 RASR_A1;
	_rw u32 RBAR_A2;
	_rw u32 RASR_A2;
	_rw u32 RBAR_A3;
	_rw u32 RASR_A3;
} mpu_reg;

/*
 * Multimedia card interface registers
 */
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

/*
 * Gigabit MAC SA registers
 */
typedef struct {
	_rw u32 SAB;
	_rw u32 SAT;
} gmac_sa;

/*
 * Gigabit MAC ST2CW registers
 */
typedef struct {
	_rw u32 ST2CW0;
	_rw u32 ST2CW1;
} gmac_st2cw;

/*
 * Gigabit MAC registers
 */
typedef struct {
	_rw u32 NCR;
	_rw u32 NCFGR;
	_r  u32 NSR;
	_rw u32 UR;
	_rw u32 DCFGR;
	_rw u32 TSR;
	_rw u32 RBQB;
	_rw u32 TBQB;
	_rw u32 RSR;
	_r  u32 ISR;
	_w  u32 IER;
	_w  u32 IDR;
	_rw u32 IMR;
	_rw u32 MAN;
	_r  u32 RPQ;
	_rw u32 TPQ;
	_rw u32 TPSF;
	_rw u32 RPSF;
	_rw u32 RJFML;
	_r  u32 RESERVED1[13];
	_rw u32 HBR;
	_rw u32 HRT;
	gmac_sa SA[4];
	_rw u32 TIDM1;
	_rw u32 TIDM2;
	_rw u32 TIDM3;
	_rw u32 TIDM4;
	_rw u32 WOL;
	_rw u32 IPGS;
	_rw u32 SVLAN;
	_rw u32 TPFCP;
	_rw u32 SAMB1;
	_rw u32 SAMT1;
	_r  u32 RESERVED2[3];
	_rw u32 NCS;
	_rw u32 SCL;
	_rw u32 SCH;
	_r  u32 EFTSH;
	_r  u32 EFRSH;
	_r  u32 PEFTSH;
	_r  u32 PEFRSH;
	_r  u32 RESERVED3[2];
	_r  u32 OTLO;
	_r  u32 OTHI;
	_r  u32 FT;
	_r  u32 BCFT;
	_r  u32 MFT;
	_r  u32 PFT;
	_r  u32 BFT64;
	_r  u32 TBFT127;
	_r  u32 TBFT255;
	_r  u32 TBFT511;
	_r  u32 TBFT1023;
	_r  u32 TBFT1518;
	_r  u32 GTBFT1518;
	_r  u32 TUR;
	_r  u32 SCF;
	_r  u32 MCF;
	_r  u32 EC;
	_r  u32 LC;
	_r  u32 DTF;
	_r  u32 CSE;
	_r  u32 ORLO;
	_r  u32 ORHI;
	_r  u32 FR;
	_r  u32 BCFR;
	_r  u32 MFR;
	_r  u32 PFR;
	_r  u32 BFR64;
	_r  u32 TBFR127;
	_r  u32 TBFR255;
	_r  u32 TBFR511;
	_r  u32 TBFR1023;
	_r  u32 TBFR1518;
	_r  u32 TMXBFR;
	_r  u32 UFR;
	_r  u32 OFR;
	_r  u32 JR;
	_r  u32 FCSE;
	_r  u32 LFFE;
	_r  u32 RSE;
	_r  u32 AE;
	_r  u32 RRE;
	_r  u32 ROE;
	_r  u32 IHCE;
	_r  u32 TCE;
	_r  u32 UCE;
	_r  u32 RESERVED4[2];
	_rw u32 TISUBN;
	_rw u32 TSH;
	_r  u32 RESERVED5[3];
	_rw u32 TSL;
	_rw u32 TN;
	_w  u32 TA;
	_rw u32 TI;
	_r  u32 EFTSL;
	_r  u32 EFTN;
	_r  u32 EFRSL;
	_r  u32 EFRN;
	_r  u32 PEFTSL;
	_r  u32 PEFTN;
	_r  u32 PEFRSL;
	_r  u32 PEFRN;
	_r  u32 RESERVED6[28];
	_r  u32 RXLPI;
	_r  u32 RXLPITIME;
	_r  u32 TXLPI;
	_r  u32 TXLPITIME;
	_r  u32 RESERVED7[96];
	_r  u32 ISRPQ[5];
	_r  u32 RESERVED8[11];
	_rw u32 TBQBAPQ[5];
	_r  u32 RESERVED9[11];
	_rw u32 RBQBAPQ[5];
	_r  u32 RESERVED10[3];
	_rw u32 RBSRPQ[5];
	_r  u32 RESERVED11[2];
	_rw u32 CBSCR;
	_rw u32 CBSISQA;
	_rw u32 CBSISQB;
	_r  u32 RESERVED12[14];
	_rw u32 ST1RPQ[4];
	_r  u32 RESERVED13[12];
	_rw u32 ST2RPQ[8];
	_r  u32 RESERVED14[40];
	_w  u32 IERPQ[5];
	_r  u32 RESERVED15[3];
	_w  u32 IDRPQ[5];
	_r  u32 RESERVED16[3];
	_rw u32 IMRPQ[5];
	_r  u32 RESERVED17[35];
	_rw u32 ST2ER[4];
	_r  u32 RESERVED18[4];
	gmac_st2cw ST2CW[24];
} gmac_reg;

/*
 * Timer channel registers
 */
typedef struct {
	_w  u32 CCR;
	_rw u32 CMR;
	_rw u32 SMMR;
	_r  u32 RAB;
	_r  u32 CV;
	_rw u32 RA;
	_rw u32 RB;
	_rw u32 RC;
	_r  u32 SR;
	_w  u32 IER;
	_w  u32 IDR;
	_r  u32 IMR;
	_rw u32 EMR;
	_r  u32 RESERVED[3];
} timer_channel;

/*
 * Timer module registers
 */
typedef struct {
	timer_channel channel[3];
	_w  u32 BCR;
	_rw u32 BMR;
	_w  u32 QIER;
	_w  u32 QIDR;
	_r  u32 QIMR;
	_r  u32 QISR;
	_rw u32 FMR;
	_r  u32 RESERVED[2];
	_rw u32 WPMR;
} timer_reg;

/*
 * DMA channel registers
 */
typedef struct {
	_w  u32 CIE;
	_w  u32 CID;
	_r  u32 CIM;
	_r  u32 CIS;
	_rw u32 CSA;
	_rw u32 CDA;
	_rw u32 CNDA;
	_rw u32 CNDC;
	_rw u32 CUBC;
	_rw u32 CBC;
	_rw u32 CC;
	_rw u32 CDS_MSP;
	_rw u32 CSUS;
	_rw u32 CDUS;
	_r  u32 RESERVED[2];
} dma_channel_reg;

/*
 * DMA registers
 */
typedef struct {
	_r  u32 GTYPE;
	_rw u32 GCFG;
	_rw u32 GWAC;
	_w  u32 GIE;
	_w  u32 GID;
	_r  u32 GIM;
	_r  u32 GIS;
	_w  u32 GE;
	_w  u32 GD;
	_r  u32 GS;
	_rw u32 GRS;
	_rw u32 GWS;
	_w  u32 GRWS;
	_w  u32 GRWR;
	_w  u32 GSWR;
	_r  u32 GSWS;
	_w  u32 GSWF;
	_r  u32 RESERVED[3];
	dma_channel_reg channel[24];
} dma_reg;

/*
 * USB device DMA
 */
typedef struct {
	_rw u32 DEVDMANXTDSC;
	_rw u32 DEVDMAADDRESS;
	_rw u32 DEVDMACONTROL;
	_rw u32 DEVDMASTATUS;
} usb_device_dma;

/*
 * USB host DMA
 */
typedef struct {
	_rw u32 HSTDMANXTDSC;
	_rw u32 HSTDMAADDRESS;
	_rw u32 HSTDMACONTROL;
	_rw u32 HSTDMASTATUS;
} usb_host_dma;

/*
 * USB registers
 */
typedef struct {

	/* Device register */
	_rw u32 DEVCTRL;
	_r  u32 DEVISR;
	_w  u32 DEVICR;
	_w  u32 DEVIFR;
	_r  u32 DEVIMR;
	_w  u32 DEVIDR;
	_w  u32 DEVIER;
	_rw u32 DEVEPT;
	_r  u32 DEVFNUM;
	_r  u32 RESERVED1[55];

	_rw u32 DEVEPTCFG[10];
	_r  u32 RESERVED2[2];
	_r  u32 DEVEPTISR[10];
	_r  u32 RESERVED3[2];
	_w  u32 DEVEPTICR[10];
	_r  u32 RESERVED4[2];
	_rw u32 DEVEPTIFR[10];
	_r  u32 RESERVED5[2];
	_rw u32 DEVEPTIMR[10];
	_r  u32 RESERVED6[2];
	_w  u32 DEVEPTIER[10];
	_r  u32 RESERVED7[2];
	_w  u32 DEVEPTIDR[10];
	_r  u32 RESERVED8[50];
	usb_device_dma device_dma[7];
	_r  u32 RESERVED9[32];

	/* Host registers */
	_rw u32 HSTCTRL;
	_r  u32 HSTISR;
	_w  u32 HSTICR;
	_w  u32 HSTIFR;
	_r  u32 HSTIMR;
	_w  u32 HSTIDR;
	_w  u32 HSTIER;
	_rw u32 HSTPIP;
	_rw u32 HSTFNUM;
	_rw u32 HSTADDR1;
	_rw u32 HSTADDR2;
	_rw u32 HSTADDR3;
	_r  u32 RESERVED10[52];

	_rw u32 HSTPIPCFG[10];
	_r  u32 RESERVED11[2];
	_r  u32 HSTPIPISR[10];
	_r  u32 RESERVED12[2];
	_w  u32 HSTPIPICR[10];
	_r  u32 RESERVED13[2];
	_w  u32 HSTPIPIFR[10];
	_r  u32 RESERVED14[2];
	_r  u32 HSTPIPIMR[10];
	_r  u32 RESERVED15[2];
	_w  u32 HSTPIPIER[10];
	_r  u32 RESERVED16[2];
	_w  u32 HSTPIPIDR[10];
	_r  u32 RESERVED17[2];
	_rw u32 HSTPIPINRQ[10];
	_r  u32 RESERVED18[2];
	_rw u32 HSTPIPERR[10];
	_r  u32 RESERVED19[26];
	usb_host_dma host_dma[7];
	_r  u32 RESERVED20[32];

	/* General USB control */
	_rw u32 CTRL;
	_r  u32 SR;
	_w  u32 SCR;
	_w  u32 SFR;
} usb_reg;

/*
 * UTMI registers
 */
typedef struct {
	_r  u32 RESERVED1[4];
	_rw u32 OHCIICR;
	_r  u32 RESERVED2[7];
	_rw u32 CKTRIM;
} utmi_reg;

/*
 * Defines the base address of the SOC peripherals
 */
#define CLOCK    ((clock_reg *)0x400E0600)
#define FLASH    ((flash_reg *)0x400E0C00)
#define WATCHDOG ((watchdog_reg *)0x400E1850)
#define SYSTICK  ((systick_reg *)0xE000E010)
#define NVIC     ((nvic_reg *)0xE000E100)
#define ICM      ((icm_reg *)0x40048000)
#define MATRIX   ((matrix_reg *)0x40088000)
#define DRAM     ((dram_reg *)0x40084000)
#define SCB      ((scb_reg *)0xE000ED00)
#define MPU      ((mpu_reg *)0xE000ED90)
#define CACHE    ((cache_reg *)0xE000EF50)
#define MMC      ((mmc_reg *)0x40000000)
#define GMAC     ((gmac_reg *)0x40050000)
#define GPIOA    ((gpio_reg *)0x400E0E00)
#define GPIOB    ((gpio_reg *)0x400E1000)
#define GPIOC    ((gpio_reg *)0x400E1200)
#define GPIOD    ((gpio_reg *)0x400E1400)
#define GPIOE    ((gpio_reg *)0x400E1600)
#define USART0   ((usart_reg *)0x40024000)
#define USART1   ((usart_reg *)0x40028000)
#define USART2   ((usart_reg *)0x4002C000)
#define TIMER0   ((timer_reg *)0x4000C000)
#define TIMER1   ((timer_reg *)0x40010000)
#define TIMER2   ((timer_reg *)0x40014000)
#define TIMER3   ((timer_reg *)0x40054000)
#define USB      ((usb_reg *)0x40038000)
#define UTMI     ((utmi_reg *)0x400E0400)

/*
 * Defines the address of the Cortex-M7 vecotr table offset register
 */
#define VECTOR_TABLE_BASE (u32)0xE000ED08

/*
 * Defines the address of which the kernel will be loaded. This does
 * not equal the vector table base address of the kernel.
 */
#define KERNEL_IMAGE_ADDR (u32)0x00404000

#endif
