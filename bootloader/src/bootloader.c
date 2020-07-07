#include "bootloader.h"
#include "flash.h"
#include "panic.h"
#include "print.h"
#include "frame.h"
#include "serial.h"
#include "clock.h"
#include "cpu.h"
#include "hardware.h"
#include "sections.h"
#include "memory.h"
#include "hash.h"

/// This structure will be stored in the hash flash region inside the 
/// bootloader. It must contain the size so that the any recumputation of the 
/// hash know what the initial size was
struct hash {
	u8 digest[32];
	u32 size;
};

/// Total size of the kernel, or more accurate the size of the erase block 
/// allocated to the kernel
volatile u32 kernel_total_size = 0;

/// Defines where the next page will programmed into flash; that is the relative
/// offset from the kernel base address at 0x00404000. This can be modified by
/// using CMD 0x06
volatile u32 kernel_page = 0;

/// Temporarily buffer for flash write operations
static u8 flash_buffer[512];

/// Boot signature in RAM to force the bootloader
__bootsig__ u8 boot_signature[32];

/// This data will be stored inside the bootloader info sector
__image_info__ struct image_info image_info = {
    .major_version    = 2,
    .minor_version    = 0,

    .bootloader_start = 0x00400000,
    .bootloader_size  = 0x4000,
    .bootloader_info  = 0x00403E00,

    .kernel_start     = 0x00404000,
    .kernel_size      = 0x1FC000,
    .kernel_info      = 0x00404000
};

/// Temporarily hash table location in RAM
__attribute__((aligned(128))) struct hash kernel_hash;

/// Hash table flash location
__hash_table__ const u8 hash_table_flash[32];

/// Clear the boot signature 
void clear_boot_signature(void) {
    u8* dest = (u8 *)boot_signature;    
    for (u8 i = 0; i < 32; i++) {
        *dest++ = 0x00;
    }
}

/// Check if a boot trigger is present in the boot signature area. Returns `1` 
/// if the CPU has to stay in the bootlaoder
u8 check_boot_signature(void) {

    // Boot trigger
    const char boot_trigger[16] = "StayInBootloader";

    const u8* src1 = (const u8 *)boot_signature;
    const u8* src2 = (const u8 *)boot_trigger;

    for (u8 i = 0; i < 16; i++) {
        if (*src1 != *src2) {
            return 0;
        }
        src1++;
        src2++;
    }
    return 1;
}

/// Compares the bootloader info structure (last bootloader sector) against
/// the kernel info structure (first kernel sector), and returns `1` if the 
/// address fields match
u8 check_info_match(void) {
    struct image_info* kernel_info = (struct image_info *)(image_info.kernel_info);
    
    if ((kernel_info->bootloader_start == image_info.bootloader_start) &&
        (kernel_info->bootloader_size == image_info.bootloader_size) && 
        (kernel_info->bootloader_info == image_info.bootloader_info) &&
        (kernel_info->kernel_start == image_info.kernel_start) &&
        (kernel_info->kernel_size == image_info.kernel_size) &&
        (kernel_info->kernel_info == image_info.kernel_info)) {

        return 1;
    } else {
        return 0;
    }
}   

/// Erase `size` bytes from flash starting at address 0x00404000
u8 erase_kernel_image(const u8* data) {
    
    // The erase flash command sends the size to erase in the first four bytes
    // in the payload
    u32 erase_size = 0;
    erase_size |= data[0];
    erase_size |= data[1] << 8;
    erase_size |= data[2] << 16;
    erase_size |= data[3] << 24;

    // Erase the flash starting from address 0x00404000
    u32 flash_status = flash_erase_image(erase_size);

    // Update the kernel size field. This is needed in the hash computation
    // stage
    if (flash_status) {
        kernel_total_size = flash_status;
    } else {
        return 0;
    }

    return 1;
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

/// Computes the kernel hash and stores it in the bootloader hash sector. It
/// returns `1` if the computation and flash write operation was successful. The
/// hash computation allways starts at address 0x00404000 and last `size` bytes
u8 store_kernel_hash(void) {
    // Compute the hash digest into the temporarily RAM location
    hash256_generate((void *)0x00404000, kernel_total_size, kernel_hash.digest);
    kernel_hash.size = kernel_total_size;

    // Load the flash buffer with the hash structure
    const u8* src = (const u8 *)&kernel_hash;
    u8* dest = (u8 *)flash_buffer;

    for (u32 i = 0; i < 512; i++) {
        if (i < sizeof(struct hash)) {
            *dest = *src;
        } else {
            *dest = 0xFF;
        }
        src++;
        dest++;
    }

    // Write the flash buffer to the hash sector (s30)
    if (flash_erase_write(30, flash_buffer) == 0) {
        return 0;
    }

    // Verify the flash
    if (!memory_compare(flash_buffer, (u8 *)(0x00400000 + 512 * 30), 512)) {
        return 0;
    }
    return 1;
}

/// Computes the hash of the kernel used the size field at sector 30 in flash
/// and compares the result with the digest at sector 30. If they match it 
/// returns `1`
u8 verify_kernel_hash(void) {
    // The `struct hash` must be fetched from the flash sector
    const struct hash* flash_hash = (const struct hash*)(0x00400000 + 512 * 30);

    // The size field in the hash sector must be a nonzero value larger than
    // 32 bytes
    if ((flash_hash->size == 0) || (flash_hash->size == 0xFFFFFFFF)) {
        return 0;
    }

    // Recompute the hash
    hash256_generate((void *)0x00404000, flash_hash->size, kernel_hash.digest);

    // Check the the digests match
    for (u8 i = 0; i < 32; i++) {
        if (kernel_hash.digest[i] != flash_hash->digest[i]) {
            return 0;
        }
    }
    return 1;
}

/// Sets the current write position of the kernel
void set_flash_write_offset(const u8* data) {
    kernel_page  = 0;
    kernel_page |= data[0];
    kernel_page |= data[1] << 8;
    kernel_page |= data[2] << 16;
    kernel_page |= data[3] << 24;
}

/// Releases the bootloader resources, relocated the vector table and jumps to 
/// the kernel defined by the two first entries in the vector table at address
/// 0x00404200
void start_kernel(void) {

    // Free the resources used in the bootloader
    print_flush();
    print_deinit();

    clock_tree_reset();

    // Flash operations does still have wait states
    flash_set_access_cycles(1);

    // Disable all interrupt except fault interrupts and system interrupts
    cpsid_i();

    // Disable all used peripheral clocks
    peripheral_clock_disable(10);
    peripheral_clock_disable(32);

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
