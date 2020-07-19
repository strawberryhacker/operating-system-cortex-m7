/* Copyright (C) StrawberryHacker */

#ifndef SPRINT_H
#define SPRINT_H

#include "types.h"
#include <stdarg.h>

uint16_t print_to_buffer_va(char* buffer, const char* input, va_list obj);

#endif
