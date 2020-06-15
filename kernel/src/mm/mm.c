#include "mm.h"
#include "debug.h"
#include "panic.h"

#include <stddef.h>

#define MM_ALIGN 8

/// These variables will point to the start and end of the allocated heap 
/// section. They are retrieved from the linker script.
extern u32 _heap_s;
extern u32 _heap_e;

/// Operation on the `mm_node` size field
#define MM_GET_REGION(size) (((size) >> 29) & 0b111)
#define MM_SET_REGION(size, physmem) (((size) & ~(0b111 << 29)) | \
                                     ((physmem) << 29))

#define MM_GET_SIZE(size) ((size) & 0xFFFFFFF)
#define MM_SET_SIZE(size, new_size) (((size) & ~0xFFFFFFF) | (new_size))

#define MM_IS_USED(size) ((size) & (1 << 28))
#define MM_SET_USED(size) ((size) | (1 << 28))
#define MM_SET_FREE(size) ((size) & ~(1 << 28))

/// Configure all regions which will be used by the memory alloctor. Only one 
/// memory region should contain `start_addr` and `end_addr` equal zero, in this
/// case the address pointers will be retrieved from the linker.
struct physmem sram = {
    .start_addr = 0x0,
    .end_addr   = 0x0,
    .name       = "SRAM",
    .min_alloc  = 32
};

struct physmem dram_bank_1 = {
    .start_addr = 0x70000000,
    .end_addr   = 0x700FFFFF,
    .name       = "DRAM_BANK_1",
    .min_alloc  = 12
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

/// This list must contain all memory sections used. The order of the elements 
/// MUST match with `physmem_e`. The last element has to be NULL.
struct physmem* regions[5] = {
    &sram,
    &dram_bank_1,
    &dram_bank_2_1k,
    &dram_bank_2_4k,
    NULL
};

/// This will initialize all the physical memory used by the allocator. After 
/// this function the `root_obj` will point to the first valid node in the 
/// physical memory, which will contain the entrie memory size. 
void mm_init(void) {
    u8 index = 0;
    while (regions[index] != NULL) {
        // Make a pointer to the current region
        struct physmem* curr_region = regions[index];

        // If the start address and end address is zero we need to update the
        // pointers with the `_heap_s` and `_heap_e` given from the linker
        if ((curr_region->start_addr == 0) && (curr_region->end_addr == 0)) {
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

        // Add a `mm_node` at the start and at the end of the physical memory
        struct mm_node* node_start = (struct mm_node *)curr_region->start_addr;
        struct mm_node* node_end = (struct mm_node *)(curr_region->end_addr - 
            sizeof(struct mm_node));
        
        // Update the total size of the memory including the first node, but
        // excluding the last node. This will be the`size`field of the first 
        // node
        curr_region->size = (curr_region->end_addr - curr_region->start_addr -
            sizeof(struct mm_node));

        // Update the allocated size
        curr_region->allocated = 0;

        // The first descriptor oject will not contain any data so the size 
        // should be zero, and the next field will point to the first physical 
        // node
        curr_region->root_obj.size = 0;
        curr_region->root_obj.next = node_start;

        // Remap the physical memory pointers
        curr_region->last_node = node_end;
        curr_region->root_node = &curr_region->root_obj;
        
        // Update the conteint of the first physical node
        node_start->next = node_end;
        node_start->size = MM_SET_SIZE(0, curr_region->size);
        node_start->size = MM_SET_REGION(node_start->size, index);

        // In the `first fit` allocator algorithm the last node has to point 
        // to NULL with size `0` 
        node_end->next = NULL;
        node_end->size = 0;

        index++;
    }
}

/// Takes in a FREE `mm_node` and inserts that node into the free list. It also
/// combines adjacent blocks. `first` should be a pointer to the pysical memory
/// root object.`last` should be a pointer to the last node
void mm_list_insert(struct mm_node* node, struct mm_node* first, 
    struct mm_node* last) {
    
    // This will never happend
    if (first == NULL) {
        panic("DEscriptor is NULL");
    }

    struct mm_node* iter = first;
    while (iter->next < node) {
        iter = iter->next;
    }

    // We know that `iter` is pointing to the node right before the block 
    // to insert. Also, the `iter->next` will point to the descriptor right
    // after the block to insert
    u32 iter_size = MM_GET_SIZE(iter->size);
    u32 insert_size = MM_GET_SIZE(node->size);

    // Check if the block before the node to insert is overlapping with the 
    // node to insert. If so, combine the blocks and move the node pointer to 
    // the previous block.
    if ((u32)iter + iter_size == (u32)node) {
        iter->size = MM_SET_SIZE(iter->size, iter_size + insert_size);
        node = iter;
    }

    // Check if the `node` (which may be the node to insert or an extended 
    // previous block) overlaps with the `next node` (iter->next)
    u32 next_size = MM_GET_SIZE(iter->next->size);
    u32 node_size = MM_GET_SIZE(node->size);
    if ((u32)node + node_size == (u32)iter->next) {
        // Check if the `iter->next` is the last block. In this case, the 
        // pointer `iter->next->next` cannot be retreived. The node should be
        // pointing to the last node.
        if (iter->next != last) {
            node->size = MM_SET_SIZE(node->size, node_size + next_size);
            node->next = iter->next->next;
        } else {
            node->next = last;
        }
    } else {
        node->next = iter->next;
    }

    // If the node to insert is not combined with the last block, the linked 
    // list is broken. In this case `node` will not be at the `iter` address. 
    if (node != iter) {
        iter->next = node;
    }
}

/// Takes in the size of an allocation and a physical memory to allocate from, 
/// and return a pointer to a memory block bigger or equal to `size`. 
void* mm_gp_alloc(u32 size, enum physmem_e physmem_e) {
    
    // Get a pointer to the physical memory. This will include all the 
    // information about the region, nodes and size. 
    struct physmem* physmem = regions[physmem_e];

    // This pointer will be given to the user
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
    iter->size = MM_SET_USED(iter->size);
    iter->size = MM_SET_REGION(iter->size, physmem_e);
    return_ptr = (void *)((u32)iter + sizeof(struct mm_node));

    u32 curr_block_size = MM_GET_SIZE(iter->size);

    // Remove the current block from the list
    iter_prev->next = iter->next;

    if (size + physmem->min_alloc <= curr_block_size) {
        // The `iter` node can contain more than the requested memory
        struct mm_node* new_node = (struct mm_node *)((u32)iter + size);
        new_node->size = (physmem_e << 29) | ((curr_block_size - size) & 
            0xFFFFFFF);

        physmem->allocated += size;
        iter->size = MM_SET_SIZE(iter->size, size);
        mm_list_insert(new_node, physmem->root_node, physmem->last_node);
    } else {
        physmem->allocated += curr_block_size;
    }

    return return_ptr;
}

/// If the user wants custom allocation functions he can add it here
void* mm_alloc(u32 size, enum physmem_gp region) {
    return mm_gp_alloc(size, region);
}

void* mm_alloc_4k(u32 count) {
    return mm_gp_alloc(count * 4096, GP_DRAM_BANK_2_4k);
}

void* mm_alloc_1k(u32 count) {
    return mm_gp_alloc(count * 1024, GP_DRAM_BANK_2_1k);   
}

/// Takes in a pointer and frees the memory if used
void mm_free(void* memory) {
    // Add the block to the free list
    if (memory == NULL) {
        panic("Trying to free NULL");
    }
    // Note! Check the desc address. It should be bus-accessible
    struct mm_node* node = (struct mm_node *)((u32)memory - 
        sizeof(struct mm_node));

    u8 physmem = MM_GET_REGION(node->size);
    struct physmem* region = regions[physmem];
    
    if (MM_IS_USED(node->size)) {
        // OK the memory is used
        mm_list_insert(node, region->root_node, region->last_node);
        region->allocated -= MM_GET_SIZE(node->size);
    }
}

/// Returns the total size of the current pytsical memory
u32 mm_get_size(enum physmem_e physmem) {
    return regions[physmem]->size;
}

/// Returns the total allocated size of the current pytsical memory
u32 mm_get_alloc(enum physmem_e physmem) {
    return regions[physmem]->allocated;
}

/// Returns the free size of the current pytsical memory
u32 mm_get_free(enum physmem_e physmem) {
    u32 alloc = regions[physmem]->allocated;
    u32 total = regions[physmem]->size;
    return (total - alloc);
}

/// Returns the fragmentation of the current pytsical memory
u32 mm_get_frag(enum physmem_e physmem) {
    return 0;
}