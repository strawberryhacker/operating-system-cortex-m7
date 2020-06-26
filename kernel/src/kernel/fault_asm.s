/// Copyright (C) StrawberryHacker

.syntax unified
.cpu cortex-m7
.thumb

.global hard_fault

.section .text
.global hard_fault_handler
.type hard_fault_handler, %function 

hard_fault_handler:
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp

    bl hard_fault

    bx lr
