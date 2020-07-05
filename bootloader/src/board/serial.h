/// Copyright (C) StrawberryHacker

#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

void serial_init(void);

void serial_deinit(void);

void serial_print(const char* data, ...);

void serial_printl(const char* data, ...);

void serial_flush(void);

u8 serial_read(void);

#endif
