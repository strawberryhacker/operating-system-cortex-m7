/// Copyright (C) StrawberryHacker

#include "system_call.h"
#include "print.h"
#include "thread.h"
#include "gpio.h"
#include "panic.h"

/// Syscall arguments will automaticall be placed in the registers R0-R3 so 
// the svc instructions can be called right away
void NAKED NOINLINE syscall_thread_sleep(u32 ms) {
    asm volatile ("svc #1 \n\t");
    asm volatile ("bx lr");
}

void NAKED NOINLINE syscall_gpio_toggle(gpio_reg* port, u8 pin) {
    asm volatile ("svc #2 \n\t");
    asm volatile ("bx lr");
}

void* NAKED NOINLINE syscall_mm_alloc(u32 size, enum physmem_e region) {
    asm volatile("svc #3 \n\t");
    asm volatile ("bx lr");
}

void NAKED NOINLINE syscall_mm_free(void* ptr) {
    asm volatile ("svc #4 \n\t");
    asm volatile ("bx lr");
}

/// Core SVC handler which does the unstacking of the SVC argument and function
/// parameters
void svc_handler_ext(u32* stack_ptr) {
    // This functions is called from the SVC exception handler. Therefore the 
    // `stack_ptr` points to the base of the exception stack frame:
    //
    //    0   1   2   3   4    5   6   7
    //    R0, R1, R2, R3, R12, LR, PC, xPSR
    //                             ^
    // Registers R0-R3 are used for parameter passing. The PC hold the next
    // instruction before the SVC handler. Therefore the svc #n will be located
    // at byte address PC - 2. Since the processor uses little endian, that
    // address is two bytes back from the value pointed to by PC. Therefore svc
    // argument is *PC - 2
    u8 svc = *((u8 *)stack_ptr[6] - 2);

    switch (svc) {
        case 1 : {
            thread_sleep((u64)stack_ptr[0]);
            break;
        }
        case 2 : {
            gpio_toggle((gpio_reg *)stack_ptr[0], (u8)stack_ptr[1]);
            break;
        } 
        case 3 : {
            stack_ptr[0] = (u32)mm_alloc(stack_ptr[0], (enum physmem_e)stack_ptr[1]);
            break;
        }
        case 4 : {
            mm_free((void *)stack_ptr[0]);
            break;
        }
    }
}
