#include "ringbuffer.h"
#include "memory.h"
#include "panic.h"

void ringbuffer_init(struct ringbuffer* rb, u8* buffer, u32 size)
{
    rb->buffer_start = buffer;
    rb->buffer_end = buffer + size;

    rb->read_ptr = buffer;
    rb->write_ptr = buffer;
}

/* 
 * Adds one element to the ringbuffer
 */
void ringbuffer_add(struct ringbuffer* rb, u8 data)
{
    /* Write the data */
    *(rb->write_ptr) = data;
    
    /* Increase the write pointer */
    rb->write_ptr++;

    if (rb->write_ptr == rb->read_ptr) {
         panic("Buffer overflow");
    }

    if (rb->write_ptr == rb->buffer_end) {
        rb->write_ptr = rb->buffer_start;
    }
}

/* Fix this not right */
u8 ringbuffer_read(struct ringbuffer* rb)
{
    u8 data = 0;
    if (rb->read_ptr != rb->write_ptr) {
        data = *(rb->read_ptr);
        rb->read_ptr++;
        if (rb->read_ptr == rb->buffer_end) {
            rb->read_ptr = rb->buffer_start;
        }
    }
    return data;
}

u32 ringbuffer_read_mult(struct ringbuffer* rb, u8* data, u32 size) {
    
    u32 ret_size = 0;

    while (size--) {

        if (rb->read_ptr == rb->write_ptr) {
            break;
        }
        *data = *(rb->read_ptr);

        ret_size++;
        data++;
        rb->read_ptr++;
    }
    return ret_size;
}
