#include "types.h"
#include "syscall.h"
#include "hardware.h"

volatile char string[32] = "Hello world";

//void __libc_init_array(void);

int main() {
    while (1) {
        syscall_thread_sleep(500);
        syscall_gpio_toggle(GPIOC, 8);
    }
 }