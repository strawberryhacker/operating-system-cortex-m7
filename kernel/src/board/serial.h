#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"
#include "hardware.h"

void serial_init(void);

void serial_deinit(void);

void print(const char* data, ...);

u8 serial_read(void);

#endif