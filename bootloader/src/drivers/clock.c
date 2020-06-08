#include "clock.h"
#include "hardware.h"

/// Enables one of the present clock sources 
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

/// Disables a clock source from running. Returns `0` if the clock can not
/// be disabled (it is used in the main clock net)
u8 clock_source_disable(enum clock_source source) {

    // Check that the clock is not used as the main clock source
    if (((CLOCK->MOR >> 24) & 0b1) == source) {
        return 0;
    }

    // Disable the clock
    u32 reg = CLOCK->MOR;
    reg |= 0x00370000;
    if (source == RC_OSCILLCATOR) {
        reg &= ~(1 << 3);
    } else {
        reg &= ~(1 << 0);
    }
    CLOCK->MOR = reg;
    return 1;
}

/// Routes one of the clock sources to the main clock net
void main_clock_select(enum clock_source source) {
    u32 reg = CLOCK->MOR;
    reg &= ~(1 << 24);
    
    if (source == CRYSTAL_OSCILLATOR) {
        reg |= (1 << 24);
    }
    CLOCK->MOR = reg | 0x00370000;

    // Wait for main clock selection to complete
    while (!(CLOCK->SR & (1 << 16)));
}

/// The main clock net is allways the source to the PLLA. For maximum stability
/// choose a high startup time. 
void plla_init(u8 div, u16 mul, u8 startup_time) {

    // Write PLLA configuretion register. Only one register configures the PLLA
    CLOCK->PLLA = (1 << 29) | (((mul - 1) & 0x7FF) << 16) | div | 
        ((startup_time & 0b111111) << 8);

    // Wait for the PLL to lock loop
    while (!(CLOCK->SR & (1 << 1)));
}

/// Disables the PLL. Returns `0` is the PLLA cannot be disabled (it is used)
u8 plla_disable(void) {
    // The PLLA is used as master clock source
    if ((CLOCK->MCKR & 0b11) == 2) {
        return 0;
    }

    // Clear the MUL and DIV field to turn off the PLL
    CLOCK->PLLA = (1 << 29) | (0b111111 << 8);
    return 1;
}

/// Configures the master clock controller. Takes in clock net and applies a 
/// presclaer which will yield the CPU and SysTick clock, and a divider which 
/// yields the peripherl and bus clocks.
void master_clock_select(enum clock_net net, enum master_presc presc, 
    enum master_divider div) {

    u32 reg;

    // The datasheet specifies different programming sequences for slow clock
    // sources rather to fast clock sources. 
    if ((net == SLOW_CLOCK) || (net == MAIN_CLOCK)) {
        // Write CSS
        reg = CLOCK->MCKR;
        reg &= ~0b11;
        reg |= net;
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
        reg |= net;
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
    }
}

/// Returns is the master clock is stable
u8 master_clock_verify(void) {
    return (CLOCK->SR & (1 << 3));
}

/// These functions enables or disables a peripheral clock. The `per_id` number
/// can be found in the SAMe70 datasheet at page 67 under peripherals.
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