/// Copyright (C) StrawberryHacker

.syntax unified
.cpu cortex-m7
.thumb

.global pendsv_test

.extern curr_thread;
.extern next_thread;

.section .text
.global pendsv_handler
.type pendsv_handler, %function

/// The PendSV handler is a software triggered interrupt, which is triggered 
/// after the scheduler has picked the next thread to run. It is configured with
/// the lowest priority and will execute when all other interrupt is done 
/// executing.
/// This exeption will perform the context switch between two threads. Since 
/// it is running in an exception, the exeption will automatically stack some
/// registers. This is known as a stack frame.  
pendsv_handler:
	// Get the PSP which the threads are using
	mrs r0, psp
	isb

	stmdb r0!, {r4-r11}

	ldr r1, =curr_thread   // 0x20400000
	ldr r2, [r1]		   // 0x70100000
	str r0, [r2]

	// After this point the `curr_thread` will be the same as
	// the `next_thread`
	ldr r0, =next_thread   // 0x20400000
	ldr r2, [r0]           // 0x70100000
	str r2, [r1]

	ldr r1, [r0]
	ldr r2, [r1]

	ldmia r2!, {r4-r11}
	msr psp, r2
	isb
    
    bx lr


.section .text
.global sched_run
.type sched_run, %function

/// This function will setup the core registers for the first thread to run
/// `next_thread`, and it will change the stack pointer since we are in thread mode
sched_run:
	ldr r1, =next_thread
	ldr r0, [r1]
	ldr r1, [r0]

	// The threads should use PSP
	mov r0, #2
	msr control, r0
	isb
	

	msr psp, r1
	isb

	// Setup the core registers for the first thread
	pop {r4-r11}
	pop {r0-r3}
	pop {r12}

	add sp, sp, #4
	pop {lr}
	add sp, sp, #4

	dsb
	isb

	cpsie f

    bx lr
