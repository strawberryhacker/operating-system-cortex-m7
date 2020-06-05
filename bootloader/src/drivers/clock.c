#include "clock.h"
#include "hardware.h"

void clock_source_enable(enum clock_source source) {
    if (source == RC_OSCILLCATOR) {
        // Enable the internal RC oscillator
        CLOCK->MOR |= (1 << 3) | 0x00370000;

        // Wait for the RC oscillator to stabilize
        while (!(CLOCK->SR & (1 << 17)));

        // Set RC frequency to 12 MHz
        u32 reg = CLOCK->MOR;
        reg &= ~(0b111 << 4);
        reg |= (2 << 4);
        CLOCK->MOR = reg | 0x00370000;

        // Wait for the RC oscillator to stabilize
        while (!(CLOCK->SR & (1 << 17)));
    } else {
        // Enable the crystal
        CLOCK->MOR |= (1 << 0) | 0x00370000;

        // Wait for the crystal to stabilize
        while (!(CLOCK->SR & (1 << 0)));
    }
}

void main_clock_select(enum clock_source source) {
    u32 reg = CLOCK->MOR;
    reg &= ~(1 << 24);
    
    if (source == EXTERNAL_CRYSTAL) {
        reg |= (1 << 24);
    }
    CLOCK->MOR = reg | 0x00370000;

    // Wait for main clock selection to complete
    while (!(CLOCK->SR & (1 << 16)));
}

void plla_init(u8 div, u16 mul, u8 startup_time) {
    CLOCK->PLLA = (1 << 29) | (((mul - 1) & 0x7FF) << 16) | div | 
        ((startup_time & 0b111111) << 8);

    // Wait for the PLL to lock loop
    while (!(CLOCK->SR & (1 << 1)));
}

void master_clock_select(enum clock_net clock_source, enum master_presc presc, 
    enum master_divider div) {

    u32 reg;
    if ((clock_source == SLOW_CLOCK) || (clock_source == MAIN_CLOCK)) {
        // Write CSS
        reg = CLOCK->MCKR;
        reg &= ~0b11;
        reg |= clock_source;
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        // Write PRESC
        reg = CLOCK->MCKR;
        reg &= ~(0b111 << 4);
        reg |= (presc << 4);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
    } else {
        // Write PRESC
        reg = CLOCK->MCKR;
        reg &= ~(0b111 << 4);
        reg |= (presc << 4);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        // Write DIV
        reg = CLOCK->MCKR;
        reg &= ~(0b11 << 8);
        reg |= (div << 8);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        // Write CSS
        reg = CLOCK->MCKR;
        reg &= ~(0b11);
        reg |= clock_source;
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
    }
}

void peripheral_clock_enable(u8 per_id) {
    if (per_id < 32) {
        CLOCK->PCER0 = (1 << per_id);
    } else if (per_id < 64){
        per_id -= 32;
        CLOCK->PCER1 = (1 << per_id);
    } else {
        // Can not use fast access.
        CLOCK->PCR = (per_id & 0b1111111) | (1 << 12) | (1 << 28);
    }
}

void peripheral_clock_disable(u8 per_id) {
    if (per_id < 32) {
        CLOCK->PCDR0 = (1 << per_id);
    } else if (per_id < 64){
        per_id -= 32;
        CLOCK->PCDR1 = (1 << per_id);
    } else {
        // Can not use fast access.
        CLOCK->PCR = (per_id & 0b1111111) | (1 << 12);
    }
}