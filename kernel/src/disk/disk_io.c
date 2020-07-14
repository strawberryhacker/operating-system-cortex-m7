/* Copyright (C) StrawberryHacker */

#include "disk_io.h"
#include "sd.h"
#include "sd_protocol.h"

u8 disk_get_status(enum disk disk) {
	switch (disk) {
		case DISK_SD_CARD: {
			return sd_is_connected();
			break;
		}
	}
	return 0;
}

u8 disk_initialize(enum disk disk) {
	switch (disk) {
		case DISK_SD_CARD: {
			sd_protocol_init();
			return 1;
			break;
		}
	}
	return 0;
}

u8 disk_read(enum disk disk, u8* buffer, u32 lba, u32 count) {
	switch (disk) {
		case DISK_SD_CARD: {
			return sd_read(lba, count, buffer);
			break;
		}
	}
	return 0;
}

u8 disk_write(enum disk disk, const u8* buffer, u32 lba, u32 count) {
	switch (disk) {
		case DISK_SD_CARD: {
			return sd_write(lba, count, buffer);
			break;
		}
	}
	return 0;
}