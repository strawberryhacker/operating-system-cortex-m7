# Bootloader

This is a short introduction to how to bootloader works.

## Host interface

Some kind of communication interface is needed in order to download the kernel from a host computer. The communication protocol used is a custom fram oriented protocol

| Start byte | Command | Size | Payload | FCS | End byte |
|-|-|-|-|-|-|
| 8 bits | 8 bits | 32 bits | up to 512 bytes | 8 bits | 8 bits |

The **start byte** marks the start of the frame. It is normally set to 0xAA. If the start byte is received while the bus is idle, the timeout logic is enabled and the frame FSM is started. 

The **command** field contains an 8 bit number indicating what type of packet being sent.

The **size** field is encoded as little-endian and specify the size of the frame payload. It can technically support longer payloads than 512 but is limited by the device buffer size.

The **payload** contains the data that will be sent. This field together with the command field determines what the packet will be used for. 

The **FCS** is an 8 bit cyclic redundancy check appended after every payload. **NOTE** that the FCS is calculated over the following fields; command, size and payload. The start and end byte is NOT included.

The **end byte** marks the end of transmission. When the device receives this character it will compute and check the FCS and determine if the frame can be used of if a retransmission should occur. 

The host also expect a response after every packet. This is due to the time concuming process of writing and erasing flash. In addition, the host cannot send a frame without the device being ready. The responce looks like this

| Response |
|-|
| 8 bit |

Some of these bits are used for required action and some are used to report status

- **bit 0** - if this bit is set a failure has occured
- **bit 1** - if this bit is set the host should perform a retransmission of the current frame
- **bit 2** - if this bit is set the host should exit the firmware upgrade
- **bit 3** - indicates a flash error
- **bit 4** - indicates a FCS error
- **bit 5** - indicates that a timeout has occured
- **bit 6** - no meaning
- **bit 7** - no meaning

## Security

The bootloader must verify the kernel image before jumping to it. If this has been modified by the kernel or an uncomplete firmware upgrade the bootloader will not load the image. For this, a hardware SHA-256 digest is computed and stored in flash after a firmware upgrade. Before any jump to the kernel, the SHA-256 digest is recomputed and compared with the flash digest.

## Memory


