#ifndef HOST_INTERFACE_H
#define HOST_INTERFACE_H

#include "types.h"

#define PACKET_START 0b10101010

struct packet_s {
	u8 cmd;
	u8 crc;
	u16 size;
	u8 payload[512];
};

enum state_e {
	STATE_IDLE,
	STATE_CMD,
	STATE_SIZE,
	STATE_PAYLOAD,
	STATE_CRC
};

enum command_e {
	CMD_LAST_PACKET = 1,
	CMD_ERASE_FLASH
};

enum error_s {
	NO_ERROR,
	CRC_ERROR,
	FLASH_ERROR,
	COMMAND_ERROR
};

void host_ack(void);

#endif