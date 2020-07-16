/* Copyright (C) StrawberryHacker */

.syntax unified
.cpu cortex-m7
.thumb

.global hard_fault

/* Extracts the SP which was used before the exception */
.section .text
.global hard_fault_exception
.type hard_fault_exception, %function 

hard_fault_exception:
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp

    bl hard_fault

    bx lr
