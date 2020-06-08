#ifndef CPU_H
#define CPU_H

#include "types.h"

/// If a funciton is marked with this attribute, the function is stored in 
/// flash and relocated into RAM in the startup. All flash functions should 
/// be marked with this attribute. 
#define __ramfunc__ __attribute__((long_call, section(".ramfunc")))

/// Reserved for bootloader mechanics in the jump condition
#define __bootsig__ __attribute__((section(".boot_signature")))

/// Reserved for bootloader mechanics in the image hader verification
#define __header__ __attribute__((section(".header")))

#endif