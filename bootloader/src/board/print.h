/* Copyright (C) StrawberryHacker */

#ifndef PRINT_H
#define PRINT_H

#include "types.h"

void print_init(void);

void print_deinit(void);

void print(const char* data, ...);

void printl(const char* data, ...);

void print_flush(void);

#endif
