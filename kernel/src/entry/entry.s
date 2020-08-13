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
	str r0, [r1]

	/* Branch to main */
	bl __libc_init_array
	bl main
inf_loop:
	b inf_loop


/*
 * Default exception alias for unused interrupts. If interrupts are
 * enabled and triggered without the exception being declared, this
 * functions is executed
 */

.global exception_fault

.section .text, "ax", %progbits
default_exception:
	bl exception_fault
	b default_exception

/*
 * Definition of the Cortex-M7 vector table
 */
.section .vector_table, "a", %progbits
.type Vector_table, %object
Vector_table:
    .word	_stack_e
	.word	entry

	/* Cortex-M7 core interrupts */
	.word	nmi_exception          /* NMI */
	.word	hard_fault_exception   /* Hard fault */
	.word	mem_fault_exception    /* Memory fault */
	.word	bus_fault_exception    /* Bus fault */
	.word	usage_fault_exception  /* Usage fault  */
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	svc_exception          /* SVC */
	.word	default_exception
	.word	default_exception
	.word	pendsv_exception       /* PendSV */
	.word	systick_exception      /* Systick */

	/* Paripheral interrupts */
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	usart0_exception      /* USART0 */
	.word	usart1_exception      /* USART1 */
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	timer0_ch0_exception  /* Timer 0 */
	.word	default_exception
	.word	default_exception
	.word	timer1_ch0_exception  /* Timer 1 */
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	usb_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	gmac_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	timer3_ch0_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception
	.word	default_exception

	/*
	 * If any interrupt are enabled but the exception not present, it
	 * will run the default exception in an infinite loop
	 */
	.weak systick_exception
	.thumb_set systick_exception, default_exception

	.weak usart1_exception
	.thumb_set usart1_exception, default_exception

	.weak usart0_exception
	.thumb_set usart0_exception, default_exception

	.weak nmi_exception
	.thumb_set nmi_exception, default_exception

	.weak hard_fault_exception
	.thumb_set hard_fault_exception, default_exception

	.weak mem_fault_exception
	.thumb_set mem_fault_exception, default_exception

	.weak bus_fault_exception
	.thumb_set bus_fault_exception, default_exception

	.weak usage_fault_exception
	.thumb_set usage_fault_exception, default_exception

	.weak svc_exception
	.thumb_set svc_exception, default_exception

	.weak pendsv_exception
	.thumb_set pendsv_exception, default_exception

	.weak gmac_exception
	.thumb_set gmac_exception, default_exception

	.weak timer0_ch0_exception
	.thumb_set timer0_ch0_exception, default_exception

	.weak usb_exception
	.thumb_set usb_exception, default_exception

	.weak timer3_ch0_exception
	.thumb_set timer3_ch0_exception, default_exception

	.weak timer1_ch0_exception
	.thumb_set timer1_ch0_exception, default_exception
