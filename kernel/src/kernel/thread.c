/* Copyright (C) StrawberryHacker */

#include "thread.h"
#include "scheduler.h"
#include "mm.h"
#include "print.h"
#include "cpu.h"
#include "panic.h"
#include "memory.h"
#include "cache.h"

#include <stddef.h>

/*
 * The main CPU runqueue structure
 */
extern struct rq cpu_rq;

/*
 * Holdes the current kernel tick times the systick reload value register
 */
extern volatile struct thread* curr_thread;

/*
 * This function will be placed in the LR to all threads. That way
 * all exiting threads go through this function
 */
void thread_exit(void) {
    printl("Exiting thread");
    print_flush();
    curr_thread->exit_pending = 1;
    while (1);
}

u32* stack_setup(u32* stack_pointer, void(*thread)(void*), void* arg) {
    /* Top padding */
    stack_pointer--;

    /* Set the Thumb bit in the xPSR register */
    *stack_pointer-- = 0x1000000;

    /* Set the PC */
    *stack_pointer-- = (u32)thread;

    /* Set the LR */
    *stack_pointer-- = (u32)thread_exit;

    /* Setup the rest of the stack frame */
    *stack_pointer-- = 0xCAFECAFE;          /* R12 */
    *stack_pointer-- = 0xCAFECAFE;          /* R3  */
    *stack_pointer-- = 0xCAFECAFE;          /* R2  */
    *stack_pointer-- = 0xCAFECAFE;          /* R1  */
    *stack_pointer-- = (u32)arg;            /* R0  */

    /*
     * The following registers will be manually stacked by the 
     * stmdb instruction in the context switch. The link register
     * has to stay on the top. The link register (or EXC_RETURN
     * in an exception handler) holds the stacking status when 
     * entering an exception handler
     */

    /* Setup the EXC_RETURN with no FPU and PSP usage*/
    *stack_pointer-- = 0xFFFFFFFD;

    /* Setup the rest of the processor registers */
    *stack_pointer-- = 0xCAFECAFE;          /* R11 */
    *stack_pointer-- = 0xCAFECAFE;          /* R10 */
    *stack_pointer-- = 0xCAFECAFE;          /* R9  */
    *stack_pointer-- = 0xCAFECAFE;          /* R8  */
    *stack_pointer-- = 0xCAFECAFE;          /* R7  */
    *stack_pointer-- = 0xCAFECAFE;          /* R6  */
    *stack_pointer-- = 0xCAFECAFE;          /* R5  */
    *stack_pointer   = 0xCAFECAFE;          /* R4  */
	
	return stack_pointer;
}

/*
 * Adds a new thread to the sceduler and enqueues is using its
 * designated scheduling class
 */
tid_t new_thread(struct thread_info* thread_info) {
    static tid_t tid = 1;
    suspend_scheduler();

    /*
     * Compute how many 1k pages are needed to store the stack and
     * the thread control block
     */
    u32 size = sizeof(struct thread) + thread_info->stack_size * 4;
    u32 size_1k = size / 1024;
    if (size % 512) {
        size_1k++;
    }
	
    /* Allocate the stack and thread control block */
    struct thread* thread = (struct thread *)mm_alloc(size, SRAM);

    /* Calculate the stack base and the new stack pointer */
    thread->stack_base = (u32 *)((u8 *)thread + sizeof(struct thread));
    thread->stack_pointer = thread->stack_base + thread_info->stack_size - 1;
	thread->stack_pointer = stack_setup(thread->stack_pointer, 
        thread_info->thread, thread_info->arg);
	
    /*
     * The threads list node must reference the thread object. Otherwise
     * it will be meaningless to iterate through lists
     */
    thread->rq_node.obj = thread;
    thread->thread_node.obj = thread;

    /*
     * Each thread is assigned to a scheduling class, which can be
     * changed later. This is used for enqueuing the thread in a
     * running queue
     */
    if (thread_info->class == REAL_TIME) {
        thread->class = &rt_class;
    } else if (thread_info->class == APPLICATION) {
        thread->class = &app_class;
    } else if (thread_info->class == BACKGROUND) {
        thread->class = &background_class;
    } else if (thread_info->class == IDLE) {
        thread->class = &idle_class;
    }

    /*
     * Initialize the `dlist` pointers. Accurding to specification
     * these MUST point to NULL
     */
    thread->rq_node.next = NULL;
    thread->rq_node.prev = NULL;
    thread->thread_node.next = NULL;
    thread->thread_node.prev = NULL;

    thread->rq_list = NULL;

    /* Assign a name to the thread */
    thread->name_len = string_len(thread_info->name);
    memory_copy(thread_info->name, thread->name, THREAD_MAX_NAME_LEN);

    /* Enqueue the thread using its designated scheduling class */
    thread->class->enqueue(thread, &cpu_rq);

    /* Add the thread into the global thread list */
    dlist_insert_first(&thread->thread_node, &cpu_rq.threads);

    /* Initializing variables used for runtime stats */
    thread->tick_to_wake = 0;
    thread->runtime_curr = 0;
    thread->runtime_new = 0;

    /* Assign a thread ID number */
    thread->tid = tid++;

    thread->exit_pending = 0;

    /* Update the code addr field */
    thread->code_addr = thread_info->code_addr;

    icache_invalidate();
    dcache_clean();        // Clean and invalidate

    resume_scheduler();

    dmb();
    dsb();
    isb();

	return thread->tid;
}

void thread_sleep(u64 ms) {
    /*
     * Calculate the tick to wake. The timebase will be the same as
     * the systick reload value register
     */
    curr_thread->tick_to_wake = get_kernel_tick() + ms * SYSTICK_RVR;
    
    /* This will enqueue the thread in the sleep list */
    scheduler_enqueue_delay((struct thread *)curr_thread);

    reschedule();
}

void kill_thread(tid_t tid) {
    suspend_scheduler();

    struct thread* th = get_thread(&cpu_rq, tid);

    if (th) {
        th->exit_pending = 1;
        print("Killing %8s\n", th->name);
    } else {
        print("Thread does not exist");
    }

    resume_scheduler();
}
