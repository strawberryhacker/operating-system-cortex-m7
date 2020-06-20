#include "types.h"
#include "syscall.h"

char string[32] = "Hello world";

void __libc_init_array(void);

int main() {
    __libc_init_array();
    *((volatile u8 *)0x80000000) = 90;
    while (1) {
        for (u8 i = 0; i < 32; i++) {
            (volatile void)string[i];
        }
    }
 }