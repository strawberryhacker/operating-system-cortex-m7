/* Copyright (C) StrawberryHacker */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "types.h"

#define MAX_PAYLOAD_SIZE 512

/* Response codes */
#define RESP_OK          (u8)(0 << 0)
#define RESP_ERROR       (u8)(1 << 0)
#define RESP_RETRANSMIT  (u8)(1 << 1)
#define RESP_HOST_EXIT   (u8)(1 << 2)
#define RESP_FLASH_ERROR (u8)(1 << 3)
#define RESP_FCS_ERROR   (u8)(1 << 4)
#define RESP_TIMEOUT     (u8)(1 << 5)
#define RESP_FRAME_ERROR (u8)(1 << 6)

/// This struct will contain useful information from the frame. The struct
/// has to be packed because the FCS is calulated from multiple fields so the 
/// compiler can not add padding
struct __attribute__((packed)) frame {
    u8  cmd;
    u16 size;
    u8  payload[MAX_PAYLOAD_SIZE];
    u8  fcs;
};

void bootloader_init(void);

void send_response(u8 error_code);

u8 check_new_frame(void);

#endif
