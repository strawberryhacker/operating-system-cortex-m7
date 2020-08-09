/* Copyright (C) StrawberryHacker */

#ifndef PRAND_H
#define PRAND_H

#include "types.h"

/*
 * prand is using a psudorandom algorithm that is supposed to be very fast. 
 * However, this uses 28 time-units while the true random number generator uses
 * 11 time units. 
 */

void prand_init(void);

u32 prand(void);

#endif