#include "types.h"
#include "syscall.h"
#include "hardware.h"

int main(void) {
    
    while (1) {
        syscall_thread_sleep(500);
        syscall_gpio_toggle(GPIOC, 8);
        syscall_gpio_toggle(GPIOC, 8);
        syscall_gpio_toggle(GPIOC, 8);
    }

    return 1;
 }
