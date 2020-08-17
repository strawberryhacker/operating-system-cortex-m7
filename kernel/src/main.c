/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fpi.h"


#include <stddef.h>

static void print_thread(void* args)
{
	while (1) {
		//print("SOF\n");
		syscall_thread_sleep(500);
	}
}

int main(void)
{
	kernel_entry();

	/* Programming interface for dynamic fetching of applications */
	struct thread_info fpi_info = {
		.name       = "FPI",
		.stack_size = 256,
		.thread     = fpi,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	struct thread_info print_info = {
		.name       = "Info",
		.stack_size = 256,
		.thread     = print_thread,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	new_thread(&fpi_info);
	new_thread(&print_info);
	
	scheduler_start();
}
