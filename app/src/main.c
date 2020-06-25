#include "types.h"
#include "syscall.h"
#include "hardware.h"

volatile char string[32] = "Hello world";

void __libc_init_array(void);

volatile u8 buffer[10];

void func(void) {
    for (volatile u8 i = 0; i < 10; i++) {
        (void)buffer[i];
    }
}

int main(void) {

    while (1) {
        syscall_thread_sleep(500);
        syscall_gpio_toggle(GPIOC, 8);
    }
 }

