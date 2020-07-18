#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "types.h"


/*
 * Ringbuffer struct supporting 8-bit entris
 */
struct ringbuffer {
    u8* buffer_start;
    u8* buffer_end;

    u8* read_ptr;
    u8* write_ptr;
};


/*
 * Initializes a ringbuffer
 */
void ringbuffer_init(struct ringbuffer* rb, u8* buffer, u32 size);

void ringbuffer_add(struct ringbuffer* rb, u8 data);

u8 ringbuffer_read(struct ringbuffer* rb);

u32 ringbuffer_read_mult(struct ringbuffer* rb, u8* data, u32 size);

#endif