#include "print.h"
#include "sprint.h"
#include "syscall.h"

char buffer[256];

/*
 * Prints the specified string which optional formatting
 */
void print(const char* data, ...)
{
    /* Format the buffer */
    va_list obj;
    va_start(obj, data);
    u32 size = print_to_buffer_va(buffer, data, obj);
    va_end(obj);

    const char* ptr = buffer;
    while (size--) {
        u8 status;
        do {
            status = syscall_print_get_status();
        } while (!status);

        syscall_print_byte(*ptr);
        ptr++;
    }
}