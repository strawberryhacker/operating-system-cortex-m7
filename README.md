# Summary

Vanilla is a single-core operating system for the ARMv7 architecture. It will work on all Cortex-M7 processors from Microchip, however the SAME70 Xplained might be the best choice as it don't require any port. 

# Building

Build the bootloader by `cd bootloader && make`

Flash the bootloader binary at address 0x00400000

Change the serial port in line 92 in the Makefile

Build and upload the kernel with `cd kernel && make`


## Upcoming features

- Safe bootloader w/hash check &check;
- Python kernel programmer &check;
- Basic driver support &check;
- SD card support
- General purpose DMA core itegrated with the scheduler
- Multiclass scheduler w/FPU support and lazy stacking
- Runtime statistics
- Memory statistics
- System calls
- Locks
- Runtime program execution (.bin)
- LCD support with video playback (raw format)
- FAT32 stack
- TCP/IP stack
- Server support
- Audio interface

Everything will be completely bare metal - no libraries

# Bootloader

The software embeds a flash bootloader residing inside flash at address 0x00400000. It can open a serial connection with a host computer in order to download the new kernel. The bootloader will load the kernel image to address 0x00404000. The first page of the image must consist of the kernel header. Therefore the actual application and vector table starts at address 0x00404200. The bootloader is accessed in the following ways

- kernel info struct not matching the bootloader info struct
- kernel image not valid (SHA256 mismatch)
- boot signature at address 0x20400000 says "StayInBootloader"
- trigger the boot-pin during reset (pull GPIOA 11 low)

In order to program the chip on-the-fly the kernel should provide the same serial interface as the bootloader. If the character `\0` is received it should write the boot signature and reset itself. However, when a software failure occurs, the kernel might mot be able to process the serial interrupts. Therefore two mechanisms has been added to ensure the bootloader will be usable.

- Panic is safe to use. It will print the debug info and return straight to the bootloader
- If the programming interface is not responding, the CPU has stopped somewhere. To bypyss the image loading stage in the bootloader; hold in SW0 while resetting the board. This will start up the bootloader.
