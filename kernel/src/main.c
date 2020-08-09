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
#include "mm_benchmark.h"
#include "smalloc.h"

#include <stddef.h>

static void p(void* arg) {
	while (1) {
		syscall_thread_sleep(500);
		print("ok\n");
	}
}

int main(void)
{
	kernel_entry();

	struct smalloc_benchmark_conf c = {
		.min_block_size = 8,
		.max_block_size = 256,
		.min_block_count = 0,
		.max_block_count = 0,
		.max_mm_usage = 90,
		.min_mm_usage = 50
	};

	run_mm_benchmark(&c);

	while (1);

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

	//usb_phy_init();

	while (1);

	scheduler_start();
}
