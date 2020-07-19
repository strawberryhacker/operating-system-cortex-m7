/* Copyright (C) StrawberryHacker */

#include "clock.h"
#include "hardware.h"

/*
 * Enables one of the present clock sources
 */
void clock_source_enable(enum clock_source source, u8 startup_time) {
    if (source == RC_OSCILLCATOR) {
        /* Enable the internal RC oscillator */
        CLOCK->MOR |= (1 << 3) | 0x00370000;

        /* Wait for the RC oscillator to stabilize */
        while (!(CLOCK->SR & (1 << 17)));
    } else {
        /* Enable the crystal and set the startup time */
        u32 reg = CLOCK->MOR;
        reg &= ~((0xFF << 8) | (1 << 1));
        reg |= (1 << 0) | (startup_time << 8) | 0x00370000;
        CLOCK->MOR = reg;

        /* Wait for the crystal to stabilize */
        while (!(CLOCK->SR & (1 << 0)));
    }
}

/*
 * Disables a clock source from running. Returns `0` if the clock
 * can not be disabled (it is used in the main clock net)
 */
u8 clock_source_disable(enum clock_source source) {

    /* Check that the clock is not used as the main clock source */
    if (((CLOCK->MOR >> 24) & 0b1) == source) {
        return 0;
    }

    /* Disable the clock */
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

/*
 * Sets the frequency of the internal RC oscillator
 */
void rc_frequency_select(enum rc_frecuency frec) {
	/* The RC oscillator must be enabled  */
	if (!(CLOCK->SR & (1 << 17))) {
		/* Panic */
		while (1);
	}
	
	/* Update the frequency */
	u32 reg = CLOCK->MOR;
	reg &= ~(0b111 << 4);
	reg |= (frec << 4);
	CLOCK->MOR = reg;
	
	/* Wait for the RC oscillator to stabilize */
	while (!(CLOCK->SR & (1 << 17)));
}

/*
 * Routes one of the clock sources to the main clock net
 */
void main_clock_select(enum clock_source source) {
    u32 reg = CLOCK->MOR;
    reg &= ~(1 << 24);
    
    if (source == CRYSTAL_OSCILLATOR) {
        reg |= (1 << 24);
    }
    CLOCK->MOR = reg | 0x00370000;

    /* Wait for main clock selection to complete */
    while (!(CLOCK->SR & (1 << 16)));
}

/*
 * The main clock net is allways the source to the PLLA. For
 * maximum stability choose a high startup time.
 */
void plla_init(u8 div, u16 mul, u8 startup_time) {

    /*
     * Write PLLA configuretion register. Only one register
     * configures the PLLA
     */
    CLOCK->PLLA = (1 << 29) | (((mul - 1) & 0x7FF) << 16) | div | 
        ((startup_time & 0b111111) << 8);

    /* Wait for the PLL to lock loop */
    while (!(CLOCK->SR & (1 << 1)));
}

/*
 * Disables the PLL. Returns `0` is the PLLA cannot be disabled (it is used)
 */
u8 plla_disable(void) {
    /* The PLLA is used as master clock source */
    if ((CLOCK->MCKR & 0b11) == 2) {
        return 0;
    }

    /* Clear the MUL and DIV field to turn off the PLL */
    CLOCK->PLLA = (1 << 29) | (0b111111 << 8);
    return 1;
}

/*
 * Configures the master clock controller. Takes in clock net and
 * applies a presclaer which will yield the CPU and SysTick clock,
 * and a divider which yields the peripherl and bus clocks.
 */
void master_clock_select(enum clock_net net, enum master_presc presc, 
    enum master_divider div) {

    u32 reg;

    /*
     * The datasheet specifies different programming sequences for
     * slow clock sources rather to fast clock sources.
     */
    if ((net == SLOW_CLOCK) || (net == MAIN_CLOCK)) {
        /* Write CSS */
        reg = CLOCK->MCKR;
        reg &= ~0b11;
        reg |= net;
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        /* Write PRESC */
        reg = CLOCK->MCKR;
        reg &= ~(0b111 << 4);
        reg |= (presc << 4);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
		
		/* Write divider */
        reg = CLOCK->MCKR;
        reg &= ~(0b11 << 8);
        reg |= (div << 8);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
		
    } else {
        /* Write PRESC */
        reg = CLOCK->MCKR;
        reg &= ~(0b111 << 4);
        reg |= (presc << 4);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        /* Write DIV */
        reg = CLOCK->MCKR;
        reg &= ~(0b11 << 8);
        reg |= (div << 8);
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));

        /* Write CSS */
        reg = CLOCK->MCKR;
        reg &= ~(0b11);
        reg |= net;
        CLOCK->MCKR = reg;
        while (!(CLOCK->SR & (1 << 3)));
    }
}

/*
 * Returns is the master clock is stable
 */
u8 master_clock_verify(void) {
    return (CLOCK->SR & (1 << 3));
}

/*
 * These functions enables or disables a peripheral clock. The
 * `per_id` number can be found in the SAMe70 datasheet at page
 * 67 under peripherals.
 */
void peripheral_clock_enable(u8 per_id) {
    if (per_id < 32) {
        CLOCK->PCER0 = (1 << per_id);
    } else if (per_id < 64){
        per_id -= 32;
        CLOCK->PCER1 = (1 << per_id);
    } else {
        /* Can not use fast access. */
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
        /* Can not use fast access. */
        CLOCK->PCR = (per_id & 0b1111111) | (1 << 12);
    }
}

/* Configures one of the 8 programmable clocks */
void pck_init(enum pck pck, enum clock_net net, u8 presc) {
    CLOCK->PCK[pck] = (presc << 4) | net;
}

void pck_enable(enum pck pck) {
    CLOCK->SCER = (1 << (8 + pck));
}

void pck_disable(enum pck pck) {
    CLOCK->SCDR = (1 << (8 + pck));
}

/*
 * Resets the clock tree. Only the internal RC oscillator is allowed
 * to run. Both CPU and bus clocks are running at 12 MHz
 */
void clock_tree_reset(void) {
	master_clock_select(MAIN_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_OFF);
	plla_disable();
	main_clock_select(RC_OSCILLCATOR);
	rc_frequency_select(RC_12_MHz);
	clock_source_disable(CRYSTAL_OSCILLATOR);
}

/*
 * Sets up the UPLL for the USB tranceiver. The source is allways the 
 * either 12 or 16 MHz crystal oscillator.
 */
void upll_init(enum upll_mult mult)
{
    /* Set the multiplying factor in the USB transmitter macrocell */
    UTMI->CKTRIM = mult;

    /* Choose maximum startup time */
    CLOCK->UCKR = (0b1111 << 20) | (1 << 16);
    while (!(CLOCK->SR & (1 << 6)));

    /*
     * The UPLL is now stable so the system can configure the
     * 48MHz clock. Both the peripheral clock, 480 MHz clock and
     * the 48 MHz clock are required by the USB peripheral
     */
    CLOCK->MCKR &= ~(1 << 13);  /* UPLLDIV2 */
    while (!(CLOCK->SR & (1 << 3)));

    CLOCK->USB = (1 << 0) | (9 << 8); /* USBHS clock divided by 9 + 1 */

    CLOCK->SCER = (1 << 5);
    while (!(CLOCK->SCSR & (1 << 5)));
}
