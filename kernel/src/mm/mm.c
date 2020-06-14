#include "mm.h"
#include "debug.h"
#include "panic.h"

#include <stddef.h>

#define MM_ALIGN 8

/// Pointers gotten from the linker script
extern u32 _heap_s;
extern u32 _heap_e;

#define MM_GET_REGION(size) (((size) >> 29) & 0b111)
#define MM_SET_REGION(size, region) (((size) & ~(0b111 << 29)) | ((region) << 29))

#define MM_GET_SIZE(size) ((size) & 0xFFFFFFF)
#define MM_SET_SIZE(size, new_size) (((size) & ~0xFFFFFFF) | (new_size))

#define MM_IS_USED(size) ((size) & (1 << 28))
#define MM_SET_USED(size) ((size) | (1 << 28))
#define MM_SET_FREE(size) ((size) & ~(1 << 28))


/// Sets the 28 lower bits to the size
static inline void mm_set_size(u32* size, u32 new_size) {
    *size &= ~0xFFFFFFF;
    *size |= (new_size & 0xFFFFFFF);
}

/// Sets the three upper bits [31:29] to the region number
static inline void mm_set_region(u32* size, u8 region) {
    *size &= ~(0b111 << 29);
    *size |= (region << 29);
}

/// Configure all regions which will be used by the memory alloctor. Only one 
/// memory region should contain `start_addr` and `edn_addr` equal zero, in this
/// case the address will be retrieved from the linker.
struct mm_region sram = {
    .start_addr = 0x0,
    .end_addr = 0x0,
    .name = "SRAM",
    .min_alloc = 32
};

struct mm_region dram_bank_1 = {
    .start_addr = 0x70000000,
    .end_addr = 0x700FFFFF,
    .name = "DRAM_BANK_1",
    .min_alloc = 12
};

struct mm_region dram_bank_2 = {
    .start_addr = 0x70100000,
    .end_addr = 0x701FFFFF,
    .name = "DRAM_BANK_2",
    .min_alloc = 256
};

struct mm_region* regions[4] = { &sram, &dram_bank_1, &dram_bank_2, NULL };

void mm_init(void) {
    u8 index = 0;
    while (regions[index] != NULL) {
        // Make a pointer to the current region
        struct mm_region* curr_region = regions[index];

        if ((curr_region->start_addr == 0) && (curr_region->end_addr == 0)) {
            // The address has to be fetched from the linker script
            curr_region->start_addr = (u32)&_heap_s;
            curr_region->end_addr = (u32)&_heap_e;
        }

        // Align the start address upwards
        if (curr_region->start_addr & (MM_ALIGN - 1)) {
            curr_region->start_addr += MM_ALIGN;
            curr_region->start_addr &= ~(MM_ALIGN - 1);
        }

        // Allign the end address downwards
        if (curr_region->end_addr & (MM_ALIGN - 1)) {
            curr_region->end_addr &= ~(MM_ALIGN - 1);
        }

        // Add the descriptors
        struct mm_desc* desc_start = (struct mm_desc *)curr_region->start_addr;
        struct mm_desc* desc_end = (struct mm_desc *)(curr_region->end_addr - 
            sizeof(struct mm_desc));
        
        // Update the total size
        curr_region->size = (curr_region->end_addr - curr_region->start_addr -
            sizeof(struct mm_desc));

        // Update the allocated size
        curr_region->allocated = 0;

        // The first descriptor oject will not contain any data so the size 
        // should be zero
        curr_region->first_obj.size = 0;

        // Remap the region pointers
        curr_region->last_desc = desc_end;
        curr_region->first_desc = &curr_region->first_obj;
        curr_region->first_obj.next = desc_start;

        desc_start->next = desc_end;
        desc_start->size = MM_SET_SIZE(0, curr_region->size);
        desc_start->size = MM_SET_REGION(desc_start->size, index);
        desc_end->next = NULL;
        desc_end->size = 0;

        index++;
    }
}

void mm_list_insert(struct mm_desc* desc, struct mm_desc* first, 
    struct mm_desc* last) {
    
    if (first == NULL) {
        panic("DEscriptor is NULL");
    }

    struct mm_desc* iter = first;
    while (iter->next < desc) {
        iter = iter->next;
    }

    // We know that `mm_iter` is pointing to the descriptor right before the 
    // desciptor to insert. Also, the `mm_iter->next` will point to the 
    // descriptor right after the block to insert
    u32 iter_size = MM_GET_SIZE(iter->size);
    u32 desc_size = MM_GET_SIZE(desc->size);

    if ((u32)iter + iter_size == (u32)desc) {
        iter->size = MM_SET_SIZE(iter->size, iter_size + desc_size);
        desc = iter;
    }

    // The descriptor has been merged with the previous block
    u32 next_size = MM_GET_SIZE(iter->next->size);
    desc_size = MM_GET_SIZE(desc->size);
    if ((u32)desc + desc_size == (u32)iter->next) {
        if (iter->next != last) {
            mm_set_size(&(desc->size), desc_size + next_size);
            desc->next = iter->next->next;
        } else {
            desc->next = last;
        }
    } else {
        desc->next = iter->next;
    }

    if (desc != iter) {
        iter->next = desc;
    }
}

void* mm_alloc(u32 size, enum mm_region_e region_e) {
    struct mm_region* region = regions[region_e];

    void* return_ptr = NULL;

    size += sizeof(struct mm_desc);

    // Check if the requested memory is too small for the selected region
    if (size < region->min_alloc) {
        size = region->min_alloc;
    }

    // Align the size
    if (size & (MM_ALIGN - 1)) {
        size += MM_ALIGN;
        size &= ~MM_ALIGN;
    }

    struct mm_desc* iter = region->first_desc->next;
    struct mm_desc* iter_prev = region->first_desc;

    // Try to find a free block which is big enough
    while (iter->next != NULL) {

        if (MM_GET_SIZE(iter->size) >= size) {
            break;
        }
        iter_prev = iter;
        iter = iter->next;
    }

    if (iter->next == NULL) {
        panic("Not enough memory");
        return NULL;
    }

    // `iter` is pointing to a block which is large enough to hold the
    // memory
    iter->size = MM_SET_USED(iter->size);
    iter->size = MM_SET_REGION(iter->size, region_e);
    return_ptr = (void *)((u32)iter + sizeof(struct mm_desc));

    u32 curr_block_size = MM_GET_SIZE(iter->size);

    iter_prev->next = iter->next;

    if (size + region->min_alloc <= curr_block_size) {
        // `iter` can contain more than the requested memory
        struct mm_desc* new_desc = (struct mm_desc *)((u32)iter + size);
        new_desc->size = (region_e << 29) | ((curr_block_size - size) & 
            0xFFFFFFF);

        region->allocated += size;
        iter->size = MM_SET_SIZE(iter->size, size);
        mm_list_insert(new_desc, region->first_desc, region->last_desc);
    } else {
        region->allocated += curr_block_size;
    }

    return return_ptr;
}

void* mm_alloc_4k(u32 size);

void* mm_alloc_1k(u32 size);

void mm_free(void* memory) {
    // Add the block to the free list
    if (memory == NULL) {
        panic("Trying to free NULL");
    }
    // Note! Check the desc address. It should be bus-accessible

    struct mm_desc* desc = (struct mm_desc *)((u32)memory - sizeof(struct mm_desc));

    u8 reg_index = MM_GET_REGION(desc->size);

    struct mm_region* region = regions[reg_index];
    
    if (MM_IS_USED(desc->size)) {
        // OK the memory is used
        mm_list_insert(desc, region->first_desc, region->last_desc);
        region->allocated -= MM_GET_SIZE(desc->size);
    }
}


u32 mm_get_size(enum mm_region_e region) {
    return regions[region]->size;
}

u32 mm_get_alloc(enum mm_region_e region) {
    return regions[region]->allocated;
}

u32 mm_get_free(enum mm_region_e region) {
    u32 alloc = regions[region]->allocated;
    u32 total = regions[region]->size;
    return (total - alloc);
}

u32 mm_get_frag(enum mm_region_e region) {
    return 0;
}