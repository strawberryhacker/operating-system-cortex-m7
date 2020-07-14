/// Copyright (C) StrawberryHacker

#include "types.h"
#include "syscall.h"
#include "hardware.h"

int main(void) {
    
    // ====================================================================
    // Put the application code here. The function may return, the scheduler 
    // will take care of it. No startup code is needed. Please read the 
    // docmentation before implementing apps. 
    // ====================================================================
    char test_print[64] = "Hello world\n";
    while (1) {
        syscall_thread_sleep(500);
        syscall_gpio_toggle(GPIOC, 8);

        const char* ptr = (const char *)test_print;
		while (*ptr) {
			u8 status;
			do {
				status = syscall_print_get_status();
				syscall_thread_sleep(1);
			} while (!status);

			syscall_print_byte(*ptr);

			ptr++;
		}
        
    }

    return 1;
 }
