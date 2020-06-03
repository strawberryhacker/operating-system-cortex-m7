/// Copyright (C) StrawberryHacker

#include "systick.h"

void systick_init(void) {
    *((u32 *)0x20400000) = 5;
}