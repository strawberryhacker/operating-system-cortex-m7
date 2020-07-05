# Bootloader

This is a short introduction to how to bootloader works.

## Host interface

Some kind of communication interface is needed in order to download the kernel from a host computer. I am using a custom self-defined protocol. A short summary of the protocol is defined below.

### Frame

| Start byte | Command | Size | Payload | FCS | End byte |
|-|-|-|-|-|-|
| 8 bits | 8 bits | 16 bits | up to 512 bytes | 8 bits | 8 bits |

The **start byte** marks the start of the frame. It is normally set to 0xAA. If the start byte is received while the bus is idle, the timeout logic is enabled and the frame FSM is started. 

The **command** field contains an 8 bit number indicating what type of packet being sent.

The **size** field is encoded as little-endian and specifies the size of the frame payload. It can technically support longer payloads than 512 but is limited by the device buffer size.

The **payload** contains the data that will be sent. This field together with the command field determines what the packet will be used for. 

The **FCS** is an 8 bit cyclic redundancy check appended after every payload. **NOTE** that the FCS is calculated over the following fields; command, size and payload. The start and end byte is NOT included.

The **end byte** marks the end of transmission. When the device receives this character it will compute and check the FCS and determine if the frame is ok. If not it determines if a retransmission should occur. This byte is normally 0x55.

### Response

The host also expect a response after every packet. This is due to the time concuming process of writing and erasing flash. In addition, the host cannot send a frame without the device being ready. The responce looks like this

| Response |
|-|
| 8 bit |

Some of these bits are used for starting an action and some are used to reporting status

- **bit 0** - if this bit is set a failure has occured
- **bit 1** - if this bit is set the host should perform a retransmission of the current frame
- **bit 2** - if this bit is set the host should exit the firmware upgrade
- **bit 3** - indicates a flash error
- **bit 4** - indicates a FCS error
- **bit 5** - indicates that a timeout has occured
- **bit 6** - indicates a frame error
- **bit 7** - no meaning

## Security

The bootloader must verify the kernel image before jumping to it. If this has been modified by the kernel or an uncomplete firmware upgrade the bootloader will not load the image. For this a hardware SHA-256 digest is computed and stored in flash after the firmware upgrade. Before any jump to the kernel, the SHA-256 digest is recomputed and compared with the flash digest.

## Image info

Both the kernel and the bootloader includes one sector (512 bytes) in flash which contains information about the system partitioning. The bootloader will for example have addresses of where the bootlaoder starts and ends, and other important sectors. The main point is that this MUST match the info structure in the kernel info page. If these match the bootloader knows that the kernel has the right view on the system memory partitioning. If the kernel for example should access the bootloader version the bootloader knows that this is an allowed memory location. This info is located at the last sector of the bootloader and in the first sector of the kernel. The info looks like

```c
struct image_info {
	u32 major_version;
	u32 minor_version;
	u32 boot_start;
	u32 boot_size;
	u32 kernel_start;
	u32 kernel_max_size;
};
```

## Force bootloader

If a valid firmware exist in flash it will be loaded when the CPU is either reset or powered on. However there might be cases where the user want to bypass the firmware loading and force the chip to enter the bootloader. For example if the kernel is broken and the host kernel_prog.py can't access the chip, the user might want to force the chip to enter the bootloader so that a new firmware can fix the issue. Another case is when the kernel is running and the user want to upgrade the firmware. There is several methods to force the chip to enter bootloader mode. It will force the bootloader if

- "StayInBootloader" is written in the boot_signature RAM section
- The boot pin is held low during startup

Typically the last method is used only when the kernel doesn't respond, or has crashed. The first method will be the standard boot trigger for a firmware upgrade. The kernel_prog.py will send a firmware upgrade request to the kernel which will write "StayInBootloader" to the boot_signature memory area before it resets itself. Then either of these triggers are present the chip will not attempt loading the image, and proceed to the bootloader which will start listening for frames. 

## Working principle

When the CPU is powered on it does some basic initialization. This is comprised of a clock tree initialization and an ICM configuration. First the chip checks if it has to force the bootloader. This means checking the two triggers; boot pin and boot signature in RAM. If none of these triggers are present, the bootloader will attempt to load the kernel image. It starts by checking the info structures. If these match it computes the SHA-256 hash and compares it agains the original hash digest in flash. If either of these fails the chip will continue to the bootloader. If both test succeed the bootloader will locate the vector table, free the resources used, load the SP and PC from the vector table and jump to the kernel. 

If either the kernel loading fails or one of the boot triggers is present, the bootloader will enter frame mode. In this mode (while loop) is waits for commands (frames) from the host. These commands will be able to both erase and program the flash. It also has a way of jumping to the kernel when the firmware upgrade is complete. If this last command is received the chip will calculate the SHA-256 of the image and store it in the hash flash section. Then it will load the kernel.

*If anything is unclear please ask...*

