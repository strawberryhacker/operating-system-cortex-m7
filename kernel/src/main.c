/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fat32.h"
#include "programming.h"
#include "panic.h"
#include "ringbuffer.h"
#include "usbhc.h"
#include "usb_phy.h"
#include "gpio.h"
#include "smalloc_benchmark.h"
#include "bmalloc_benchmark.h"
#include "smalloc.h"
#include "bmalloc.h"

#include <stddef.h>

static void p(void* arg) {
	while (1) {
		syscall_thread_sleep(500);
	}
}

int main(void)
{
	kernel_entry();

	struct thread_info fpi_info = {
		.name       = "FPI",
		.stack_size = 256,
		.thread     = fpi,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	struct thread_info p_info = {
		.name       = "p",
		.stack_size = 256,
		.thread     = p,
		.class      = REAL_TIME,
		.arg        = NULL,
		.code_addr  = 0
	};

	new_thread(&fpi_info);
	new_thread(&p_info);

	run_bmalloc_benchmark();

	scheduler_start();
}
