#include "host_interface.h"
#include "serial.h"

volatile struct packet_s packet = {0};
volatile enum state_e state = STATE_IDLE;
volatile u32 packet_index = 0;
volatile u8 packet_flag = 0;

void host_ack(void) {
    print("%c", (char)NO_ERROR);
}

/// The interrupt routine will receive the kernel image in packages of 512 bytes
/// Packet:      [ start byte ] [ command ] [ size ] [ payload ] [ CRC ]
///
/// start byte - indicates start of packet. Should be set to a non-ascii char
/// command    - one byte where 0x01 is reserved and mark the final package
/// size       - two bytes indicating the size of the package. First is LSByte
/// payload    - holds the data
/// CRC        - cyclic redundancy check
void usart1_handler() {
	u8 rec_byte = serial_read();

	switch (state) {
		case STATE_IDLE : {
			// If the bootloader
			if (rec_byte == 0) {
				print("%c", (char)NO_ERROR);
			}
			if (rec_byte == PACKET_START) {
				if (packet_flag == 0) {
                    state = STATE_CMD;
                } else {
                    while (1);
                }
			}
			break;
		}
		case STATE_CMD : {
			packet.cmd = rec_byte;
			state = STATE_SIZE;
			packet.size = 0;
			packet_index = 0;
			break;
		}
		case STATE_SIZE : {
			packet.size |= (rec_byte << (8 * packet_index));
			packet_index++;
			if (packet_index >= 2) {
				state = STATE_PAYLOAD;
				packet_index = 0;
			}
			break;
		}
		case STATE_PAYLOAD : {
			packet.payload[packet_index] = rec_byte;
			packet_index++;
			if (packet_index >= packet.size) {
				state = STATE_CRC;
			}
			break;
		}
		case STATE_CRC : {
			packet.crc = rec_byte;
			state = STATE_IDLE;
			packet_flag = 1;
			break;
		}
	}
}