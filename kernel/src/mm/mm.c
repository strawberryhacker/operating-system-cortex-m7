/// Copyright (C) StrawberryHacker

#include "mm.h"
#include "panic.h"

#include <stddef.h>

/// Every memory block will have an address and a size aligned by this amount
#define MM_ALIGN 8

/// `_heap_s` and `_heap_e` defines the address space of the heap allocation.
/// They are defined in the linker script
extern u32 _heap_s;
extern u32 _heap_e;

/// Gets and sets the physical memory encoded in the upper 4 bits
#define MM_GET_REGION(size) (((size) >> 28) & 0b1111)
#define MM_SET_REGION(size, physmem) (((size) & ~(0b1111 << 28)) | \
                                     ((physmem) << 28))

/// Gets and sets the block size encoded in the lower 28 bits
#define MM_GET_SIZE(size) ((size) & 0xFFFFFFF)
#define MM_SET_SIZE(size, new_size) (((size) & ~0xFFFFFFF) | (new_size))

/// Configure all regions which will be used by the memory alloctor. Only one 
/// physical memory should ever have `start_addr` and `end_addr` equal zero. 
/// In this case the address pointers will be retrieved from the linkers heap
/// section
struct physmem sram = {
    .start_addr = 0x0,
    .end_addr   = 0x0,
    .name       = "SRAM",
    .min_alloc  = 16
};

struct physmem dram_bank_1 = {
    .start_addr = 0x70000000,
    .end_addr   = 0x700FFFFF,
    .name       = "DRAM_BANK_1",
    .min_alloc  = 256
};

struct physmem dram_bank_2_1k = {
    .start_addr = 0x70100000,
    .end_addr   = 0x7017FFFF,
    .name       = "DRAM_BANK_2",
    .min_alloc  = 256
};

struct physmem dram_bank_2_4k = {
    .start_addr = 0x70180000,
    .end_addr   = 0x701FFFFF,
    .name       = "DRAM_BANK_2",
    .min_alloc  = 256
};

/// List containing a pointer to all the physical memories. Last entry must be
/// NULL
struct physmem* physical_memories[5] = {
    &sram,
    &dram_bank_1,
    &dram_bank_2_1k,
    &dram_bank_2_4k,
    NULL
};

/// Initialize all the physical memories used by the memory allocator. After 
/// this function the `root_obj` will point to the first valid node in the 
/// physical memory. This node will contain the entire memory size
void mm_init(void) {
    u8 index = 0;
    while (physical_memories[index] != NULL) {
        struct physmem* physmem = physical_memories[index];

        // If the start address and end address is zero we need to update the
        // pointers with the `_heap_s` and `_heap_e` given from the linker
        if ((physmem->start_addr == 0) && (physmem->end_addr == 0)) {
            physmem->start_addr = (u32)&_heap_s;
            physmem->end_addr = (u32)&_heap_e;
        }

        // Align the start address upwards
        if (physmem->start_addr & (MM_ALIGN - 1)) {
            physmem->start_addr += MM_ALIGN;
            physmem->start_addr &= ~(MM_ALIGN - 1);
        }

        // Allign the end address downwards
        if (physmem->end_addr & (MM_ALIGN - 1)) {
            physmem->end_addr &= ~(MM_ALIGN - 1);
        }

        // Add a `mm_node` at the start and end of the aligned physical memory
        struct mm_node* node_start = (struct mm_node *)physmem->start_addr;
        struct mm_node* node_end = (struct mm_node *)(physmem->end_addr - 
            sizeof(struct mm_node));
        
        // Update the total size of the memory including the first node, but
        // excluding the last node. All sizes are calculated like this
        physmem->size = (physmem->end_addr - physmem->start_addr -
            sizeof(struct mm_node));

        // The first node will not contain any data hence the size should be 
        // zero, and it will point to the first physical node
        physmem->root_obj.size = 0;
        physmem->root_obj.next = node_start;

        // Remap the physical memory pointers
        physmem->last_node = node_end;
        physmem->root_node = &physmem->root_obj;
        
        node_start->next = node_end;
        node_start->size = MM_SET_SIZE(0, physmem->size);
        node_start->size = MM_SET_REGION(node_start->size, index);

        // In the `first fit` algorithm the last node is a zero-sized node
        // pointing to NULL
        node_end->next = NULL;
        node_end->size = 0;

        physmem->allocated = 0;
        index++;
    }
}

/// Inserts a `mm_node` into the free list, and combines adjacent blocks if 
/// present. `first` should be a pointer to the pysical memory root object.
/// `last` should be a pointer to the last node
void mm_list_insert(struct mm_node* node, struct mm_node* root, 
    struct mm_node* last) {

    struct mm_node* iter = root;
    while (iter->next < node) {
        iter = iter->next;
    }

    // Now `iter` is pointing to the node right before the block to insert.
    // Also, `iter->next` point to the node right after the block to insert
    u32 iter_size = MM_GET_SIZE(iter->size);
    u32 insert_size = MM_GET_SIZE(node->size);

    // Check if the block to insert overlaps with the previous free block. If 
    // so, combine the blocks and move the node pointer to the previous block.
    if ((u32)iter + iter_size == (u32)node) {
        iter->size = MM_SET_SIZE(iter->size, iter_size + insert_size);
        node = iter;
    }

    // Check if the `node` (which may be the node to insert, or the previous 
    // block; if merged) overlaps with the following block (iter->next)
    u32 next_size = MM_GET_SIZE(iter->next->size);
    u32 node_size = MM_GET_SIZE(node->size);
    if ((u32)node + node_size == (u32)iter->next) {
        
        // Check if the `iter->next` is the last block. In this case, the 
        // pointer `iter->next->next` cannot be retreived. The node should point
        // to the last node
        if (iter->next != last) {
            node->size = MM_SET_SIZE(node->size, node_size + next_size);
            node->next = iter->next->next;
        } else {
            node->next = last;
        }
    } else {
        node->next = iter->next;
    }

    // If the block to insert is not merged backwards, the `node` and `iter` 
    // have different address. Then these nodes has to be linked together
    if (node != iter) {
        iter->next = node;
    }
}

/// Allocates `size` number of bytes from a physical memory
void* mm_gp_alloc(u32 size, enum physmem_e index) {
    
    struct physmem* physmem = physical_memories[index];
    void* return_ptr = NULL;

    // The size should contain the `mm_node` 
    size += sizeof(struct mm_node);

    // Check if the requested size is bigger than the physical memory's minimum
    // allocation size
    if (size < physmem->min_alloc) {
        size = physmem->min_alloc;
    }

    // Align the size
    if (size & (MM_ALIGN - 1)) {
        size += MM_ALIGN;
        size &= ~MM_ALIGN;
    }

    struct mm_node* iter = physmem->root_node->next;
    struct mm_node* iter_prev = physmem->root_node;

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
    }

    // The `iter` is pointing to a memory block which is large enough to
    // contain the requested memory.
    iter->size = MM_SET_REGION(iter->size, index);
    u32 curr_block_size = MM_GET_SIZE(iter->size);

    return_ptr = (void *)((u32)iter + sizeof(struct mm_node));

    // Remove the current block from the list
    iter_prev->next = iter->next;

    // Check if the remaining part of the block is big enough to contain a new
    // memory block
    if (size + physmem->min_alloc <= curr_block_size) {

        struct mm_node* new_node = (struct mm_node *)((u32)iter + size);
        new_node->size = (index << 28) | ((curr_block_size - size) & 0xFFFFFFF);

        physmem->allocated += size;
        iter->size = MM_SET_SIZE(iter->size, size);

        mm_list_insert(new_node, physmem->root_node, physmem->last_node);
    } else {
        physmem->allocated += curr_block_size;
    }

    // Mark the memory as allocated
    iter->next = (struct mm_node *)0xC0DEBABE;

    if (return_ptr == NULL) {
        panic("NULL pointer returned");
    }

    return return_ptr;
}

/// Allocated `size` number of bytes from a none-reserved region. The size 
/// might still be padded according to the physical memory settings
void* mm_alloc(u32 size, enum physmem_e region) {
    // The 1k and 4k physical memories are reserved
    if ((region == DRAM_BANK_2_4k) || (region == DRAM_BANK_2_1k)) {
        panic("Wrong parameter");
    }

    return mm_gp_alloc(size, region);
}

/// Allocates a number of 4k pages
void* mm_alloc_4k(u32 count) {
    return mm_gp_alloc(count * 4096, DRAM_BANK_2_4k);
}

/// Allocates a number of 1k pages
void* mm_alloc_1k(u32 count) {
    return mm_gp_alloc(count * 1024, DRAM_BANK_2_1k);   
}

/// Free the memory pointed to by `memory`
void mm_free(void* memory) {

    if (memory == NULL) {
        panic("Trying to free NULL");
    }
    // Note! Check the desc address. It should be bus-accessible
    struct mm_node* node = (struct mm_node *)((u32)memory - 
        sizeof(struct mm_node));

    // Check if the memory pointer has been allocated by the mm_alloc*
    if ((u32)node->next != 0xC0DEBABE) {
        panic("Pointer not made by mm_alloc*");
    }

    u8 physmem = MM_GET_REGION(node->size);
    struct physmem* region = physical_memories[physmem];
    
    mm_list_insert(node, region->root_node, region->last_node);
    region->allocated -= MM_GET_SIZE(node->size);
}

/// Returns the total size of the current pytsical memory
u32 mm_get_size(enum physmem_e physmem) {
    return physical_memories[physmem]->size;
}

/// Returns the total allocated size of the current pytsical memory
u32 mm_get_alloc(enum physmem_e physmem) {
    return physical_memories[physmem]->allocated;
}

/// Returns the free size of the current pytsical memory
u32 mm_get_free(enum physmem_e physmem) {
    u32 alloc = physical_memories[physmem]->allocated;
    u32 total = physical_memories[physmem]->size;
    return (total - alloc);
}

/// Returns the fragmentation of the current pytsical memory
u32 mm_get_frag(enum physmem_e physmem) {
    return 0;
}
