/* Copyright (C) StrawberryHacker */

#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

enum clock_source {
    RC_OSCILLCATOR,
    CRYSTAL_OSCILLATOR
};

enum clock_net {
    SLOW_CLOCK,
    MAIN_CLOCK,
    PLLA_CLOCK,
    UPLL_CLOCK
};

enum master_presc {
    MASTER_PRESC_OFF,
    MASTER_PRESC_2,
    MASTER_PRESC_4,
    MASTER_PRESC_8,
    MASTER_PRESC_16,
    MASTER_PRESC_32,
    MASTER_PRESC_64,
    MASTER_PRESC_3
};

enum master_divider {
    MASTER_DIV_OFF,
    MASTER_DIV_2,
    MASTER_DIV_4,
    MASTER_DIV_3
};

enum pck {
    PCK0,
    PCK1,
    PCK2,
    PCK3,
    PCK4,
    PCK5,
    PCK6,
    PCK7
};

enum rc_frecuency {
	RC_4_MHz,
	RC_8_MHz,
	RC_12_MHz
};

/*
 * The full clock network of the chip can be found in the SAMe70 datasheet
 * at page 251
 */

void clock_source_enable(enum clock_source source, u8 startup_time);

u8 clock_source_disable(enum clock_source source);

void rc_frequency_select(enum rc_frecuency frec);

void main_clock_select(enum clock_source source);

void plla_init(u8 div, u16 mul, u8 startup_time);

u8 plla_disable(void);

void master_clock_select(enum clock_net net, enum master_presc presc, 
    enum master_divider div);

u8 master_clock_verify(void);

void peripheral_clock_enable(u8 per_id);

void peripheral_clock_disable(u8 per_id);

void pck_init(enum pck pck, enum clock_net net, u8 presc);

void pck_enable(enum pck pck);

void pck_disable(enum pck pck);

void clock_tree_reset(void);

#endif