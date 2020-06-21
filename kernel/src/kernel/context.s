/// Copyright (C) StrawberryHacker

.syntax unified
.cpu cortex-m7
.thumb

/// The `curr_thread` will allways point to the current running thread. After 
/// the context switch is will be updated by the PendSV handler. The 
/// `next_thread` should point to the next thread to run when the context switch
/// is triggered. After the context switch it is the same as `curr_thread` 
.extern curr_thread;
.extern next_thread;

/// The PendSV handler is a software triggered interrupt, which is triggered 
/// after the scheduler has picked the next thread to run. It is configured with
/// the lowest priority and will execute when all other interrupt is done 
/// executing.
/// This exeption will perform the context switch between two threads. Since 
/// it is running in an exception, the exeption will automatically stack some
/// registers. This is known as a stack frame. When the FPU is disabled the
/// exeption triggers the stacking of R0-R3, R12, LR, PC and xPSR. The exeption
/// handler stack the remainding register R4-R11 and switchs the stack pointer.
.section .text
.global pendsv_handler
.type pendsv_handler, %function 
pendsv_handler:
	// The treads uses the PSP. This has to be manually retrieved since the 
	// execption is using the MSP.
	mrs r0, psp
	isb

	stmdb r0!, {r4-r11}

	// Update the stack pointer of the current running thread
	ldr r1, =curr_thread
	ldr r2, [r1]
	str r0, [r2]

	// Make `curr_thread` point to the `next_thread`
	ldr r0, =next_thread
	ldr r2, [r0]
	str r2, [r1]

	// Get the stack pointer from the next thread to run
	ldr r1, [r2]

	ldmia r1!, {r4-r11}

	// Load the stack pointer into PSP
	msr psp, r1
	isb
    
    bx lr

/// This function will setup the core registers for the first thread to run
/// and it will change the stack pointer since the threads should run in thread
/// mode. Finally is places the stack PC into its link register. This is
/// automatically starting the first thread. The `thread_add` function sets up 
/// the stack the following way
///  -----
///   xPSR
///   PC
///   LR
///   R12
///   R3     Exception stack frame
///   R2
///   R1
///   R0
///  ----
///   R11
///   R10
///   R9
///   R8
///   R7
///   R6
///   R5
///   R4
///  ----

.section .text
.global scheduler_run
.type scheduler_run, %function
scheduler_run:
	ldr r1, =next_thread
	ldr r0, [r1]
	ldr r1, [r0]

	// The threads should use PSP
	mov r0, #2
	msr control, r0
	isb
	
	// Load the first thread's stack pointer into PSP
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
