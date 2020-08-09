/* Copyright (C) StrawberryHacker */

#include "prand.h"
#include "trand.h"

#include <stdlib.h>

void prand_init(void)
{
    trand_init();
    srand(trand());
}

u32 prand(void)
{
    return (u32)rand();
}
