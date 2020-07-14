/* Copyright (C) StrawberryHacker */

.syntax unified
.cpu cortex-m7
.thumb

/*
 * These variables are declared in the linker script. They are
 * pointing to where they are located.
 */
.extern _relocate_s
.extern _vector_table_s
.extern _vector
.extern _data_s
.extern _data_e
.extern _bss_s
.extern _bss_e
.extern _stack_e
.extern _rodata_s

/*
 * Base address of the vector table offset register
 */
vector_table_reg: .word 0xE000ED08

/*
 * Startup routine is defined in the second entry in the vector table.
 * This is loaded into PC after the chip boots at address 0x00400000.
 * This function performs relocation of the .data segment, initialization
 * of the .bss segment and vector table relocation. Finally it calls
 * `__libc_init_array` and branches to the main loop
 */
.section .text
.global entry
.type entry, %function

entry:
    /* Relocate .data segment */
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
	/* Initialize .zero segment  */
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
	/* Set vector table offset register */
	ldr r0, =_vector_table_s
	ldr r1, vector_table_reg
	bic.w r0, r0, #128
	str r0, [r1]

	/* Branch to main */
	bl __libc_init_array
	bl main
inf_loop:
	b inf_loop


/*
 * Default handler alias for unused interrupts. If interrupts are
 * enabled and triggered without the handler being declared, this
 * functions is executed
 */
.section .text, "ax", %progbits
default_handler:
	b default_handler

/*
 * Definition of the Cortex-M7 vector table
 */
.section .vector_table, "a", %progbits
.type Vector_table, %object
Vector_table:
    .word	_stack_e
	.word	entry

	/* Cortex-M7 core interrupts */
	.word	nmi_handler          /* NMI */
	.word	hard_fault_handler   /* Hard fault */
	.word	mem_fault_handler    /* Memory fault */
	.word	bus_fault_handler    /* Bus fault */
	.word	usage_fault_handler  /* Usage fault  */
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	svc_handler          /* SVC */
	.word	default_handler
	.word	default_handler
	.word	pendsv_handler       /* PendSV */
	.word	systick_handler      /* Systick */

	/* Paripheral interrupts */
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	usart0_handler      /* USART0 */
	.word	usart1_handler      /* USART1 */
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	gmac_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler
	.word	default_handler

	/*
	 * If any interrupt are enabled but the handler not present, it
	 * will run the default handler in an infinite loop
	 */
	.weak systick_handler
	.thumb_set systick_handler, default_handler

	.weak usart1_handler
	.thumb_set usart1_handler, default_handler

	.weak usart0_handler
	.thumb_set usart0_handler, default_handler

	.weak nmi_handler
	.thumb_set nmi_handler, default_handler

	.weak hard_fault_handler
	.thumb_set hard_fault_handler, default_handler

	.weak mem_fault_handler
	.thumb_set mem_fault_handler, default_handler

	.weak bus_fault_handler
	.thumb_set bus_fault_handler, default_handler

	.weak usage_fault_handler
	.thumb_set usage_fault_handler, default_handler

	.weak svc_handler
	.thumb_set svc_handler, default_handler

	.weak pendsv_handler
	.thumb_set pendsv_handler, default_handler

	.weak gmac_handler
	.thumb_set gmac_handler, default_handler
