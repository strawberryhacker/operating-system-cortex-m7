#ifndef MM_H
#define MM_H

#include "types.h"

#define MM_REGION_NAME_LENGTH 32

/// The region is encoded as three bits, allowing up to eight different regions
enum mm_region_e {
    SRAM,
    DRAM_BANK_1,
    DRAM_BANK_2
};

struct mm_desc {
    struct mm_desc* next;

    // The size bits is contiaining other information as well
    // [31:29] - mm region
    // [28]    - status (1 for used)
    u32 size;
};

struct mm_region {
    u32 start_addr;
    u32 end_addr;

    u32 size;
    u32 allocated;

    // Specify the minimum minimum allocation size. This includes 8 bytes for 
    // the memory descriptor
    u32 min_alloc;

    // Contains the name of the memory region
    char name[MM_REGION_NAME_LENGTH];

    struct mm_desc* first_desc;
    struct mm_desc first_obj;
    struct mm_desc* last_desc;
};

void mm_init(void);

/// Allocate a custom memory size
void* mm_alloc(u32 size, enum mm_region_e region);

void* mm_alloc_4k(u32 size);

void* mm_alloc_1k(u32 size);

void mm_free(void* memory);

u32 mm_get_size(enum mm_region_e region);

u32 mm_get_alloc(enum mm_region_e region);

u32 mm_get_free(enum mm_region_e region);

u32 mm_get_frag(enum mm_region_e region);

#endif