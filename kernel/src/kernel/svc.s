/// Copyright (C) StrawberryHacker

.syntax unified
.cpu cortex-m7
.thumb

/// This SVC handler will extract the SVC number and call the service function
.global svc_handler_ext
.global test_sp

.section .text
.global svc_handler
.type svc_handler, %function 

svc_handler:
    // Check which stack pointer is in use and load that into r0. This will 
    // be used as the first argument in the `svc_handler_ext` function. The 
    // parameter passing must be done through the stack and not the registers.
    // The reason is that another higher priority exception might trigger when 
    // the SVC exception is being servics. In this case the processor will 
    // tail-chain the interrupts, thus possibly overriding the values in r0-r4.
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp

    // The function will automatically return to the caller of SVC
    b svc_handler_ext
