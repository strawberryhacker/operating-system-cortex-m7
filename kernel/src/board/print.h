/// Copyright (C) StrawberryHacker

#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

void print_init(void);

void print_deinit(void);

void print(const char* data, ...);

void printl(const char* data, ...);

void print_flush(void);

#endif
