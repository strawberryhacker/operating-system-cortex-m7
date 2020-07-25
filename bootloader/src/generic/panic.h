/* Copyright (C) StrawberryHacker */

#ifndef PANIC_H
#define PANIC_H

#include "types.h"

#define panic(reason) panic_handler(__FILE__, __LINE__, (reason))

void panic_handler(const char* file_name, u32 line_number, const char* reason);

#endif