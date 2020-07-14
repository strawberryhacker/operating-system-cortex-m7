#include "run.h"
#include "memory.h"
#include "fat32.h"
#include "panic.h"
#include "mm.h"
#include "dynamic_linker.h"

#include <stddef.h>
/* 
 * Takes in a NULL terminated string and executes the binary at
 * that path
 */
u8 run_binary(const char* path) {
    u32 length = string_len(path);

    /* Open the directory */
    struct file binary_file;
    if (fat_file_open(&binary_file, path, length) != FSTATUS_OK) {
        panic("Cant open file");
    }

    /* Read the first byte to know how much to allocate */
    u32 binary_size = 0;
    u32 read_status = 0;

    if (fat_file_read(&binary_file, (u8 *)&binary_size, 4, &read_status) != FSTATUS_OK) {
        panic("Cant read");
    }
    if (read_status != 4) {
        panic("Cant read binary size");
    }

    /* Allocate enough memory to hold the entire binary file */
    u32 size_1k = binary_size / 1024;
    if (binary_size & 1024) {
        size_1k++;
    }
    u8* binary = (u8 *)mm_alloc_1k(size_1k);

    /*
     * Jump to the start of the file and read the entire binary into
     * the buffer 
     */
    fat_file_jump(&binary_file, 0);

    u8* binary_ptr = binary;
    do {
        if (fat_file_read(&binary_file, binary_ptr, 512, &read_status) != FSTATUS_OK) {
            panic("Cant read from file"); 
        }
        binary_ptr += 512;
    } while (read_status == 512);

    

    /* Link and run the executable */
    dynamic_linker_run((u32 *)binary);
}