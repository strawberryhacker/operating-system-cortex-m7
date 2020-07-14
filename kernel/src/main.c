/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "scheduler.h"
#include "kernel_entry.h"
#include "thread.h"
#include "print.h"
#include "syscall.h"
#include "fat32.h"
#include "panic.h"
#include "mm.h"
#include "sd.h"
#include "dynamic_linker.h"
#include "run.h"
#include "bootloader.h"

#include <stddef.h>

extern struct rq cpu_rq;
extern volatile struct frame frame;

static void blink_thread(void* arg) {

	char test_print[12] = "Hello World";
	while (1) {
		syscall_thread_sleep(1000);

		const char* ptr = (const char *)test_print;
		while (*ptr) {
			u8 status;
			do {
				status = syscall_print_get_status();
				syscall_thread_sleep(1);
			} while (!status);

			syscall_print_byte(*ptr);

			ptr++;
		}
		//print("Runtime:\n");
	}
}

/*
 * Used for testing the run binary
 */
static void test(void* arg) {
	syscall_thread_sleep(1000);

	run_binary("C:/bin/tictactoe.bin");

	while (1) {

	}
}

static void frame_thread(void* arg) {
	/*
	 * CMD_ALLOCATE_MEM = 0x01
     * CMD_BINARY       = 0x02
     * CMD_BINARY_LAST  = 0x03
	 */
	u8* binary_buffer = 0;
	u8* buffer_ptr = 0;
	while (1) {
		if (check_new_frame()) {
			print("New frame\n");

			if (frame.cmd == 0x01) {
				u32 size = *(u32 *)frame.payload;
				print("Allocoate size: %d\n", size);
				binary_buffer = (u8 *)mm_alloc(size, SRAM);
				buffer_ptr = binary_buffer;
			} else if ((frame.cmd == 0x02) || (frame.cmd == 0x03)) {

				for (u32 i = 0; i < frame.size; i++) {
					*buffer_ptr++ = frame.payload[i];
				}

				if (frame.cmd == 0x03) {
					dynamic_linker_run((u32 *)binary_buffer);
				}
			}
			send_response(RESP_OK);
		}
	}
}

int main(void) {

	kernel_entry();

	struct thread_info fat32_info = {
		.name       = "FAT32 thread",
		.stack_size = 1024,
		.thread     = fat32_thread,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info blink_info = {
		.name = "Blink",
		.stack_size = 256,
		.thread = blink_thread,
		.class = REAL_TIME,
		.arg = NULL
	};

	struct thread_info test_info = {
		.name = "Test",
		.stack_size = 1024,
		.thread = test,
		.class = REAL_TIME,
		.arg = NULL
	};

	struct thread_info frame_info = {
		.name = "Frame",
		.stack_size = 64,
		.thread = frame_thread,
		.class = REAL_TIME,
		.arg = NULL
	};

	//new_thread(&blink_info);
	new_thread(&fat32_info);
	//new_thread(&test_info);
	new_thread(&frame_info);

	scheduler_start();

	

	while (1);
}
