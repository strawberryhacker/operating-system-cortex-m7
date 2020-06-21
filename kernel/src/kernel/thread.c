#include "thread.h"
#include "scheduler.h"
#include "mm.h"
#include "debug.h"

void thread_exit(void) {
    while (1);
}

u32* stack_setup(u32* stack_pointer, void(*thread)(void*), void* arg) {
    // Top padding
    stack_pointer--;

    // Set the Thumb bit in the xPSR register
    *stack_pointer-- = 0x1000000;

    // Set the PC
    *stack_pointer-- = (u32)thread;

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


struct tcb* thread_add(struct thread_info* thread_info) {

    // Compute how many 1k pages are needed to store the stack and the thread
    u32 size = sizeof(struct tcb) + thread_info->stack_size * 4;
    u32 size_1k = size / 1024;
    if (size % 512) {
        size_1k++;
    }
	
    // Allocate the TCB and the stack
    struct tcb* thread = (struct tcb *)mm_alloc_1k(size_1k);

    // Calculate the stack base and the stack pointer
    thread->stack_base = (u32 *)((u8 *)thread + sizeof(struct tcb));
    thread->stack_pointer = thread->stack_base + thread_info->stack_size - 1;
	thread->stack_pointer = stack_setup(thread->stack_pointer, 
        thread_info->thread, thread_info->arg);
	
    thread->rq_node.obj = thread;

	return thread;
}
