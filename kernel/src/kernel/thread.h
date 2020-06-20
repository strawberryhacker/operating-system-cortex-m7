#ifndef THREAD_H
#define THREAD_H

#include "types.h"

struct tcb* thread_add(const char* name, void(*thread_ptr)(void* arg), u32 stack_size, u32* arg);

void thread_print_list(void);

#endif
