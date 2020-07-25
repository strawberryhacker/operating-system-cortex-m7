/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "syscall.h"
#include "hardware.h"
#include "game.h"

int main(void) {
    print("sdfkljsdflkjhdf sdlkfjh dflkjhsd lksjdh sdkljh sdflkjhsdf lkjsdhf \n");
    game_init();
    draw_board();
    game_loop();
    
    return 1;
 }
