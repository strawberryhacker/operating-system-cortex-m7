#include "game.h"
#include "print.h"
#include "memory.h"
#include "syscall.h"

char rec_buffer[16];

#define SIZE 3

/* Board data */
u8 board[SIZE * SIZE];

/* Holds the turs */
u8 turn = 1;
u8 waiting_move = 0;

/*
 * Returns the piece if the gmae has a winner. If not it returns zero
 */
static u8 check_winner(u8* data, u32 size) {
    u8 winner;

    /* Check the rows */
    for (u8 i = 0; i < size; i++) {
        u8* row_ptr = data + i * SIZE;
        winner = *row_ptr;
        for (u8 j = 0; j < SIZE; j++) {
            if (*row_ptr != winner) {
                winner = 0;
                break;
            }
            row_ptr++;
        }
        if (winner) {
            return winner;
        }
    }

    /* Check the columns */
    for (u8 i = 0; i < size; i++) {
        u8* col_ptr = data + i;
        winner = *col_ptr;
        for (u8 j = 0; j < SIZE; j++) {
            if (*col_ptr != winner) {
                winner = 0;
                break;
            }
            col_ptr += SIZE;
        }
        if (winner) {
            return winner;
        }
    }

    /* Check the diolonals */
    u8* diag_ptr = data;
    winner = *diag_ptr;
    for (u8 j = 0; j < SIZE; j++) {
        if (*diag_ptr != winner) {
            winner = 0;
            break;
        }
        diag_ptr += SIZE + 1;
    }
    if (winner) {
        return winner;
    }
    diag_ptr = data + SIZE - 1;
    winner = *diag_ptr;
    for (u8 j = 0; j < SIZE; j++) {
        if (*diag_ptr != winner) {
            winner = 0;
            break;
        }
        diag_ptr += SIZE - 1;
    }
    if (winner) {
        return winner;
    }
    return 0;
}

/*
 * Draws the board spacing between the lined
 */
static void draw_spacing(u8 size)
{
    while (size--) {
        print("|-----");
    }
    print("|\n");
}

/*
 * Draws one row in the board
 */
static void draw_row(u8* data, u8 size)
{
    while (size--) {
        char piece = ' ';
        if (*data == 1) {
            piece = 'X';
        } else if (*data == 2) {
            piece = 'O';
        }
        print("|  %c  ", piece);
        data++;
    }
    print("|\n");
}

static u8 add_piece(char* data, u8 piece)
{
    u8 x_pos = *data - '0';
    u8 y_pos = *(data + 1) - '0';

    x_pos--;
    y_pos--;

    if ((x_pos >= SIZE) || (y_pos >= SIZE)) {
        return 0;
    }

    if (board[SIZE * y_pos + x_pos] == 0) {
        board[SIZE * y_pos + x_pos] = piece;
        return 1;
    }
    return 0;
}

void draw_board(void)
{
    print("\n\n");
    for (u8 i = 0; i < SIZE; i++) {
        draw_spacing(SIZE);
        draw_row(board + i * SIZE, SIZE);
    }
    draw_spacing(SIZE);
}

void game_init(void) {
    memory_fill(board, 0, SIZE * SIZE);
    turn = 1;
    waiting_move = 0;
}

void game_loop(void)
{
    u8 winner = 0;
    do {
        if (waiting_move == 0) {
            if (turn == 1) {
                turn = 2;
            } else {
                turn = 1;
            }

            if (turn == 1) {
                print("X");
            } else if (turn == 2) {
                print("O");
            }
            print("'s turn: ");

            waiting_move = 1;
        }
        u32 status = syscall_read_print(rec_buffer, 16);

        if (status == 2) {
            if (add_piece(rec_buffer, turn)) {
                winner = check_winner(board, SIZE);

                waiting_move = 0;
                print("\n");
                draw_board(); 
            } else {
                print("\nTry again\n");
            }
        }
        syscall_thread_sleep(50);

    } while (winner == 0);

    print("Winner is %d\n", winner);
}
