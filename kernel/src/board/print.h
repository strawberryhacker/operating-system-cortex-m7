/* Copyright (C) StrawberryHacker */

#ifndef PRINT_H
#define PRINT_H

#include "types.h"

/* ANSI escape codes allow to print in colors */
#define ANSI_RED     "\033[31m"
#define ANSI_NORMAL  "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_PINK    "\033[35m"
#define BLUE         "\033[34m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_CYAN    "\033[36m"

void print_init(void);

void print_deinit(void);

void print(const char* data, ...);

void printl(const char* data, ...);

void print_raw(const char* data);

void print_memory(const u32* memory, u32 size);

void print_flush(void);

void print_count(const char* data, u32 count);

void print_byte(u8 data);

u8 print_get_status(void);

void print_register(const char* name, u32 reg);

u32 read_print_buffer(char* data, u32 size);

#endif
