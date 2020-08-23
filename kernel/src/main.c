/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fpi.h"
#include "usb_hid.h"
#include "button.h"
#include <stddef.h>

static void print_thread(void* args)
{
	while (1) {
		printl("Hello");
		syscall_thread_sleep(500);
	}
}

static tid_t tid;

static void thread_block_test(void)
{
	static u8 block = 0;
	if (block == 0) {
		block = 1;
		thread_block(tid);
	} else {
		thread_unblock(tid);
		block = 0;
	}
	printl("Blocking");
}

struct button_callback thread_block_cb = {
	.callback = &thread_block_test,
	.event = BUTTON_PRESSED
};

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

	button_add_callback(&thread_block_cb);

	new_thread(&fpi_info);
	tid = new_thread(&print_info);
	
	scheduler_start();
}
