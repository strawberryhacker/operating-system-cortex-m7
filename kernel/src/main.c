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
#include "memory.h"

#include <stddef.h>

static char* urb_id[] = {
	[0x0] = "alpha",
	[0x1] = "apple",
	[0x2] = "straw",
	[0x3] = "berry",
	[0x4] = "lemon",
	[0x5] = "lime",
	[0x6] = "oreo",
	[0x7] = "c",
	[0x8] = "tree",
	[0x9] = "car",
	[0xA] = "smoke",
};

extern struct usb_pipe usb_pipes[USB_PIPES];

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

	new_thread(&fpi_info);

	usb_phy_init();

	scheduler_start();
}
