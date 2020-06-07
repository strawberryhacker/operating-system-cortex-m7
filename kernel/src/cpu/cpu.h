#ifndef CPU_H
#define CPU_H

#include "types.h"

#define __ramfunc__ __attribute__((long_call, section(".ramfunc")))

#endif