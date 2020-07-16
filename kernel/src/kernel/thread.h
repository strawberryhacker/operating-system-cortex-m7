/* Copyright (C) StrawberryHacker */

#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "scheduler.h"

tid_t new_thread(struct thread_info* thread_info);

void thread_exit(void);

void kill_thread(tid_t tid);

void thread_sleep(u64 ms);

#endif
