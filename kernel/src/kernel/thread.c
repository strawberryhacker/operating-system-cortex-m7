#include "thread.h"
#include "scheduler.h"
#include "mm.h"
#include "debug.h"

void thread_exit(void) {
    while (1);
}

u32* stack_setup(u32* stack_pointer, void(*thread_ptr)(void* arg), void* arg) {
    // Top padding
    stack_pointer--;

    // Set the Thumb bit in the xPSR register
    *stack_pointer-- = 0x1000000;

    // Set the PC
    *stack_pointer-- = (u32)thread_ptr;

    // Set the LR
    *stack_pointer-- = (u32)thread_exit;

    // Setup the rest of the stack frame
    *stack_pointer-- = 0xCAFECAFE;          // R12
    *stack_pointer-- = 0xCAFECAFE;          // R3
    *stack_pointer-- = 0xCAFECAFE;          // R2
    *stack_pointer-- = 0xCAFECAFE;          // R1
    *stack_pointer-- = (u32)arg;   // R0

    // Setup the rest of the processor registers
    *stack_pointer-- = 0xCAFECAFE;          // R11
    *stack_pointer-- = 0xCAFECAFE;          // R10
    *stack_pointer-- = 0xCAFECAFE;          // R9
    *stack_pointer-- = 0xCAFECAFE;          // R8
    *stack_pointer-- = 0xCAFECAFE;          // R7
    *stack_pointer-- = 0xCAFECAFE;          // R6
    *stack_pointer-- = 0xCAFECAFE;          // R5
    *stack_pointer   = 0xCAFECAFE;          // R4
	
	return stack_pointer;
}


struct tcb* thread_add(const char* name, void(*thread_ptr)(void* arg), u32 stack_size, u32* arg) {
    u32 size = sizeof(struct tcb) + stack_size* 4;
    u32 size_1k = size / 1024;
    
    if (size % 512) {
        size_1k++;
    }
	
    // Allocate the TCB and the stack
    struct tcb* new_thread = (struct tcb *)mm_alloc_1k(size_1k);

    // Calculate the stack base and the stack pointer
    new_thread->stack_base = (u32 *)((u8 *)new_thread + sizeof(struct tcb));
    new_thread->stack_pointer = new_thread->stack_base + stack_size - 1;
	new_thread->stack_pointer = stack_setup(new_thread->stack_pointer, thread_ptr, arg);
	
    new_thread->rq_node.obj = new_thread;

	return new_thread;
}
