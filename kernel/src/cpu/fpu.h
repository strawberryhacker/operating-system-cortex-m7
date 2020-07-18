#ifndef FPU_H
#define FPU_H

#include "types.h"
#include "hardware.h"
#include "cpu.h"

/*
 * The Cortex-M7 CPU embeds a single and double precision FPU. It
 * supports features such as lazy stack preservation and shallow
 * stacking. It has some control registers (accessed by VMRS and
 * VMSR) which control the behaviour of the FPU
 * 
 * FPSCR  - controls FPU opertations and contains the FPU status
 * CPACR  - coprocessor access control
 * FPCCR  - controls the context saving behaviour
 * FPCAR  - holds the reserved space allocated on stack
 * FPDSCR - holds the defualt value of FPSCR
 *
 * The CONTROL.FPCA is set when the FPU is used and cleared when a
 * new context is started for example in a exception handler
 * 
 * The EXC_RETURN[4] indicates that an exception stack frame
 * contains either floating point registers, or allocated 
 * space for it
 * 
 * The FPCCR.LSPACT indicates the status of the lazy state
 * preservation. It is set to one when space is allocated on the
 * stack for FPU register, but the pushing is ignored. This 
 * can be thought of as stacking pending.
 */

/*
 * Enables full access to coprocessor 10 and 11 used by the FPU
 */
static inline void fpu_enable() {
    *(volatile u32 *)0xE000ED88 |= (0b11 << 20) | (0b11 << 22);
    dsb();
    isb();
}

#endif