/* Copyright (C) StrawberryHacker */

#include "fpi.h"
#include "bootloader.h"
#include "dynamic_linker.h"
#include "mm.h"
#include "thread.h"

/*
 * Defined in bootloader.h
 */
extern volatile struct frame frame;

/*
 * This functions polls after frames on the host interface
 * (which is shared whith the bootloader). It can recevie 
 * and start new applications directly
 */
void fpi(void* arg) {
	
	u8* binary_buffer = 0;
	u8* buffer_ptr = 0;

	tid_t curr_tid = 0;

	while (1) {
		if (check_new_frame()) {

			if (frame.cmd == 0x01) {
				u32 size = *(u32 *)frame.payload;
				binary_buffer = (u8 *)mm_alloc(size, SRAM);
				buffer_ptr = binary_buffer;
			} else if ((frame.cmd == 0x02) || (frame.cmd == 0x03)) {

				for (u32 i = 0; i < frame.size; i++) {
					*buffer_ptr++ = frame.payload[i];
				}

				if (frame.cmd == 0x03) {

					if (curr_tid) {
						kill_thread(curr_tid);
					}

					curr_tid = dynamic_linker_run((u32 *)binary_buffer);
				}
			}
			send_response(RESP_OK);
		}
	}
}