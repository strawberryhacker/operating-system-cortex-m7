#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

enum clock_source {
    RC_OSCILLCATOR,
    EXTERNAL_CRYSTAL
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

void clock_source_enable(enum clock_source source);

void main_clock_select(enum clock_source source);

void plla_init(u8 div, u16 mul, u8 startup_time);

/// The CPU clock is extracted after the prescaler
void master_clock_select(enum clock_net clock_source, enum master_presc presc, 
    enum master_divider div);

void peripheral_clock_enable(u8 per_id);

void peripheral_clock_disable(u8 per_id);

#endif