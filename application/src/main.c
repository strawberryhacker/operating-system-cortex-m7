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
    game_init();
    draw_board();
    game_loop();
    
    return 1;
 }
