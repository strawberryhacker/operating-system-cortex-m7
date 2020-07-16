/// Copyright (C) StrawberryHacker

#include "types.h"
#include "syscall.h"
#include "hardware.h"
#include "game.h"

int main(void) {
    
    // ====================================================================
    // Put the application code here. The function may return, the scheduler 
    // will take care of it. No startup code is needed. Please read the 
    // docmentation before implementing apps. 
    // ====================================================================

    draw_board();
    while (1) {
        syscall_thread_sleep(2000);
    }
    return 1;
 }
