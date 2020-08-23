<img src="https://github.com/strawberryhacker/vanilla/blob/master/doc/vanilla.png" width="300">

# Summary

Vanilla is a single-core operating system for the ARMv7 architecture. It will work on all Cortex-M7 processors from Microchip. The SAME70 Xplained board might be the best choice for testing as it don't require any port. I will make a custom board soon and explain the porting process - it's easy!

# Building

To compile this project `make` and `arm-none-eabi` is needed. In ubuntu they are installed by 
```shell
> sudo apt-get update -y
> sudo apt-get install build-essential
> sudo apt-get install -y gcc-arm-none-eabi
```

Go in the bootloader directory and type `make`

Flash the bootloader binary (in the build directory) at address 0x00400000. This can be done with Atmel Studio or OpenOCD

Line number 92 in the Makefile specify which COM port the programming script will use to connect to the board. In ubuntu it is /dev/ttySx and in windows COMx. Change this according to what COM port you use.

Go to the kernel directory and tpye `make`. This will automatically flash the chip.


## Upcoming features

- Safe bootloader w/hash check &check;
- Python kernel programmer &check;
- Basic driver support &check;
- SD card support &check;
- General purpose DMA core itegrated with the scheduler
- Multiclass scheduler w/FPU support and lazy stacking &check;
- Runtime statistics &check;
- Memory statistics 
- System calls &check;
- Locks &check;
- Runtime program execution (.bin) &check;
- Runtime program execution (.elf)
- LCD support with video playback (raw format)
- FAT32 stack &check;
- TCP/IP stack
- Server support
- Audio interface
- USB 2.0 high-speed host interface
- USB MSC with FAT32 support
- USB HID with support for mouse and keyboard
- USB Audio class
- GPU interface
- ARMv7 assembler with floating point extension
- GUI, terminals, LCD etc.
- Add thread safety
- Support mamory fault cleanup from program testing
- Dynamic MPU regions in case of thread failure
- C (my own language) compiler

Everything will be completely bare metal - no libraries - literally

# Bootloader

The software embeds a flash bootloader residing inside flash at address 0x00400000. It can open a serial connection with a host computer in order to download the new kernel. The bootloader will load the kernel to address 0x00404000. The first page of the image (512-bytes) must consist of the kernel header. Therefore the actual application and vector table starts at address 0x00404200. The bootloader is accessed in the following ways

- kernel info struct not matching the bootloader info struct
- kernel image not valid (SHA256 mismatch)
- boot signature at address 0x20400000 says "StayInBootloader"
- trigger the boot-pin during reset (pull GPIOA 11 low)

In order to program the chip on-the-fly the kernel should provide the same serial interface as the bootloader. If the character `\0` is received it should write the boot signature and reset itself. However, when a software failure occurs, the kernel might mot be able to process the serial interrupts. Therefore two mechanisms has been added to ensure the bootloader will be usable.

- Panic is safe to use. It will print the debug info and return straight to the bootloader
- If the programming interface is not responding, the CPU has stopped somewhere. To bypyss the image loading stage in the bootloader; hold in SW0 while resetting the board. This will start up the bootloader.
