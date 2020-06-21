#ifndef THREAD_H
#define THREAD_H

#include "types.h"
#include "scheduler.h"

struct tcb* thread_add(struct thread_info* thread_info);

void thread_print_list(void);

#endif
