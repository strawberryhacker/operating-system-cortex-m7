#include "bootloader.h"
#include "flash.h"
#include "panic.h"
#include "print.h"
#include "frame.h"
#include "serial.h"
#include "clock.h"
#include "cpu.h"
#include "hardware.h"

static u8 flash_buffer[512];

/// Erase `size` bytes from flash starting at address 0x00404000
u8 erase_kernel_image(const u8* data) {
    
    /// The erase flash command sends the size to erase in the first four bytes
    /// in the payload
    u32 erase_size = 0;
    erase_size |= data[0];
    erase_size |= data[1] << 8;
    erase_size |= data[2] << 16;
    erase_size |= data[3] << 24;

    printl("Erasing %d bytes from address 0x00404000", erase_size);

    /// Erase the flash starting from address 0x00404000
    u8 flash_status = flash_erase_image(erase_size);

    return flash_status;
}

/// Writes a page into the lower 8 KiB of flash and verify it after programming.
/// This function only support programming of pages with size lower or equal to 
/// 512 bytes
u8 write_bootloader_page(const u8* data, u32 size, u32 page) {

    // Check the size and add eventual 0xFF padding
    for (u32 i = 0; i < 512; i++) {
        if (i < size) {
            flash_buffer[i] = data[i];
        } else {
            flash_buffer[i] = 0xFF;
        }
    }

    // Perform the erase write
    u8 flash_status = flash_erase_write(page, data);

    if (flash_status == 0) {
        panic("Flash erase write error");
    }

    // Verify the flash
    flash_status = 1;
    const u8* flash_addr = (const u8 *)(0x00400000 + page * 512);

    for (u32 i = 0; i < 512; i++) {
        if (flash_buffer[i] != *flash_addr++) {
            flash_status = 0;
            break;
        }
    }
    // Check if the flash is ok
    if (flash_status == 0) {
        panic("Flash verification failed");
    }
    return 1;
}

/// Writes `size` bytes from `data` into flash at relative offset `page` from 
/// the kernel base address aka. 0x00404000
u8 write_kernel_page(const u8* data, u32 size, u32 page) {
    
    if (size != 512) {
        printl("Warning: non-complete page write");
    }

    // Copy to the flash buffer
    for (u32 i = 0; i < 512; i++) {
        if (i < size) {
            flash_buffer[i] = data[i];
        } else {
            flash_buffer[i] = 0xFF;
        }
    }
    // Write a page in the kernel space
    u8 flash_status = flash_write_image_page(page, flash_buffer);

    if (flash_status == 0) {
        panic("Flash write failed");
    }

    // Verify the flash
    flash_status = 1;
    const u8* flash_addr = (const u8 *)(0x00404000 + 512 * page);

    for (u32 i = 0; i < 512; i++) {
        if (flash_buffer[i] != *flash_addr++) {
            flash_status = 0;
            break;
        }
    }

    if (flash_status == 0 ) {
        panic("Flash verification error");
    }
    return 1;
}

/// Releases the bootloader resources, relocated the vector table and jumps to 
/// the kernel defined by the two first entries in the vector table at address
/// 0x00404200
void start_kernel(void) {

    // Free the resources used in the bootloader

    print("SP: %4h\n", *(volatile u32 *)0x00404200);
    print("PC: %4h\n", *(volatile u32 *)0x00404204);
    serial_flush();
    print_flush();

    frame_deinit();
    print_deinit();

    peripheral_clock_disable(12);
    clock_tree_reset();

    // Flash operations does still have wait states
    flash_set_access_cycles(1);

    // Disable all interrupt except fault interrupts and system interrupts
    cpsid_i();

    // Disable all used peripheral clocks
    //...

    // Set the vector table base address
    *((volatile u32 *)VECTOR_TABLE_BASE) = 0x00404200;

    // These barriers is mandatory after updating the vector table
    dmb();
    dsb();
    isb();

    volatile u32* image_base = (volatile u32 *)0x00404200;

    // Perform the jump to the kernel. Note that the instruction pipeline flush
    // must be done right after modifying the stack pointer. This forces the 
    // CPU to use the right stack pointer in the following instructions
    asm volatile (
        "mov r0, %0     \n\t"
        "ldr r1, [r0]   \n\t"
        "add r0, r0, #4 \n\t"
        "ldr r0, [r0]   \n\t"
        "orr r0, r0, #1 \n\t" // Set the Thumb bit
        "mov sp, r1     \n\t"
        "isb sy         \n\t" // Flushes the instruction pipeline
        "mov pc, r0     \n\t"
        : : "l" (image_base) : "r0", "r1", "memory"
	);
}
