/// Copyright (C) StrawberryHacker

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

#include <stddef.h>

extern struct rq cpu_rq;

static void blink_thread(void* arg) {
	while (1) {
		//syscall_thread_sleep(98);
		//syscall_gpio_toggle(GPIOC, 8);
		//syscall_thread_sleep(2);
		//syscall_gpio_toggle(GPIOC, 8);
		syscall_thread_sleep(1000);
		//u64 runtime = cpu_rq.idle->runtime_curr;
		print("Runtime:\n");
	}
}

static void file(void* arg) {
	// Configure the hardware
	sd_init();
	
	// Wait for the SD card to be insterted
	while (sd_is_connected() == 0);
	
	// Try to mount the disk. If this is not working the disk initialize 
	// functions may be ehh...
	if (disk_mount(DISK_SD_CARD) == 0) {
		panic("Mounting failed");
	}
	printl("Mounting OK");

	struct volume* vol = volume_get_first();

	while (vol) {
		printl("Volume: %12s (%c:)", vol->label, vol->letter);
		vol = vol->next;
	}

	struct dir dir;
	struct info* info = (struct info*)mm_alloc(sizeof(struct info), SRAM);

	if (info == NULL) {
		panic("Bad ptr");
	}

	fat_dir_open(&dir, "C:/", 3);

	fstatus status;
	print("\n");
	do {
		status = fat_dir_read(&dir, info);

		if (status == FSTATUS_OK) {
			fat_print_info(info);
		}
	} while (status == FSTATUS_OK);

	// Try to open a file
	struct file file;
	fat_file_open(&file, "C:/application.bin", 18);
	
	u32 write_status;
	u8* file_buffer = (u8 *)mm_alloc_4k(1);
	
	do {
		fat_file_read(&file, file_buffer, 4000, &write_status);
	} while (write_status == 4000);
	printl("Application is loaded into memory");
	print("App addr: %4h\n", file_buffer);
	dynamic_linker_run((u32 *)file_buffer);
	
	while (1);
}

int main(void) {

	kernel_entry();

	struct thread_info file_info = {
		.name       = "FAT32 thread",
		.stack_size = 1024,
		.thread     = file,
		.class      = REAL_TIME,
		.arg        = NULL
	};

	struct thread_info blink_info = {
		.name = "Blink",
		.stack_size = 32,
		.thread = blink_thread,
		.class = REAL_TIME,
		.arg = NULL
	};

	new_thread(&blink_info);
	new_thread(&file_info);

	scheduler_start();

}
