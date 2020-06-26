/// Copyright (C) StrawberryHacker

#ifndef DISK_INTERFACE_H
#define DISK_INTERFACE_H

#include "types.h"

/// Add new physical disk here
enum disk {
	DISK_SD_CARD
};

/// Returns the status of the MSD (mass storage device)
u8 disk_get_status(enum disk disk);

/// Initializes at disk intrface
u8 disk_initialize(enum disk disk);

/// Read a number of sectors from the MSD
u8 disk_read(enum disk disk, u8* buffer, u32 lba, u32 count);

/// Write a number of sectors to the MSD
u8 disk_write(enum disk disk, const u8* buffer, u32 lba, u32 count);

#endif