#include "game.h"
#include "print.h"
#include "memory.h"

#define SIZE 6

enum piece {
    NONE,
    CIRCLE,
    CROSS
};

static enum piece board[SIZE * SIZE] = {0};

void draw_split(void)
{
    for (u8 i = 0; i < SIZE; i++) {
        print("|-----");
    }
    print("|\n");
}

void draw_segment(const enum piece* data)
{
    for (u8 i = 0; i < SIZE; i++) {
        char p;
        if (*data == NONE) {
            p = ' ';
        } else if (*data == CIRCLE) {
            p = 'O';
        } else if (*data == CROSS) {
            p = 'X';
        }
        print("|  %c  ", p);
        data++;
    }
    print("|\n");
}

void draw_board(void)
{    
    memory_fill(board, 0, SIZE * SIZE);  

    board[3] = CIRCLE;
    board[4] = CROSS;

    for (u8 i = 0; i < SIZE; i++) {
        draw_split();
        draw_segment(board + SIZE * i);
    }
    draw_split();
}