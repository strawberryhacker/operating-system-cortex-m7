/* Copyright (C) StrawberryHacker */

#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "scheduler.h"

struct thread* new_thread(struct thread_info* thread_info);

void thread_sleep(u64 ms);

#endif
