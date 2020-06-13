/// Copyright (C) StrawberryHacker

#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

void debug_init(void);

void debug_deinit(void);

void debug_print(const char* data, ...);

void debug_flush(void);

#endif