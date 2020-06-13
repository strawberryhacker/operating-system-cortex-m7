/// Copyright (C) StrawberryHacker

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

void serial_init(void);

void serial_deinit(void);

void serial_print(const char* data, ...);

u8 serial_read(void);

#endif