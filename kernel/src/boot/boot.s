/// Copyright (C) StrawberryHacker

.syntax unified
.cpu cortex-m7
.thumb

/// Declared in the linker script
.extern _relocate_s
.extern _vector_table_s
.extern _vector
.extern _data_s
.extern _data_e
.extern _bss_s
.extern _bss_e
.extern _stack_e
.extern _rodata_s

/// Base address of the vector table
vector_table_reg: .word 0xE000ED08

/// Startup routine defined in the second entry in the vector table. This 
/// performs relocation of the .data segment, initialization of the .bss 
/// segment and vector table relocation. Finally it calls `__libc_init_array`
/// and branch to the main loop.
.section .text
.global Startup
.type Startup, %function

Startup:
    // Relocate .data segment
	ldr r0, =_data_s
	ldr r1, =_data_e
	ldr r4, =_rodata_s
	subs r2, r1, r0
	lsr r2, r2, #2
	ldr r1, =_relocate_s
	beq zero_segment

relocate_loop:
	ldr r3, [r1], #4
	str r3, [r0], #4
	subs r2, r2, #1
	bne relocate_loop

zero_segment:
	// Initialize .zero segment 
	ldr r0, =_bss_s
	ldr r1, =_bss_e
	subs r2, r1, r0
	lsr r2, r2, #2
	beq vector_table_set
	
zero_loop:
	mov r3, #0
	str r3, [r0], #4
	subs r2, r2, #1
	bne zero_loop

vector_table_set:
	// Set vector table base address
	ldr r0, =_vector_table_s
	ldr r1, vector_table_reg
	bic.w r0, r0, #128
	str r0, [r1]

	// Branch to main
	bl __libc_init_array
	bl main
inf_loop:
	b inf_loop


/// Default handler for unused interrupts
.section .text, "ax", %progbits
default_handler:
	b default_handler

/// Vector table
.section .vector_table, "a", %progbits
.type Vector_table, %object
Vector_table:
    .word	_stack_e
	.word	Startup
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	systick_handler

	// Make a default handler for unused interrupts
	.weak systick_handler
	.thumb_set systick_handler, default_handler
