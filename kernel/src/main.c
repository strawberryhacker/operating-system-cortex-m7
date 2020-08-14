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
#include "list.h"
#include "umalloc_benchmark.h"
#include "bmalloc_benchmark.h"

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

struct name {
	char name[8];
	struct list_node node;
};

void p(struct list_node* list)
{
	struct list_node* node;
	list_iterate(node, list) {
		struct name* name = list_get_entry(node, struct name, node);
		print("Node: %s\n", name->name);
	}
}

int main(void)
{
	kernel_entry();

	struct bmalloc_benchmark_conf conf = {
		/* mm_alloc */
		.min_block_size = 8,
		.max_block_size = 1024,
		.max_mm_usage = 90,
		.min_mm_usage = 30
	};


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
	
	//usb_phy_init();
	struct list_node list;

	list_init(&list);

	struct name names[10];

	for (u32 i = 0; i < 0xA; i++) {
		p(&list);
		print("\n");
		string_copy(urb_id[i], names[i].name);
		list_add_first(&names[i].node, &list);
	}

	scheduler_start();
}
