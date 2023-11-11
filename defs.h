#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    char board[8][8][3];
    char turn;
    char moves[256][4];
    int numMoves;
    int wkcastle;
    int wqcastle;
    int bkcastle;
    int bqcastle;
    int bcastled;
    int wcastled;
    int passant;
    int passant_row;
    int passant_col;
}Board;

typedef struct {
    char move[4];
    long score;
}move;

typedef struct {
    move result;
    int depth;
    Board *board;
    int function_number;
} thread_arg;

float percentage = 0;
float increment = 1;

const int pawnValue = 100;
const int knightValue = 320;
const int bishopValue = 330;
const int rookValue = 500;
const int queenValue = 900;
const int kingValue = 20000;

const int pawnTable[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5, 5, 10, 25, 25, 10, 5, 5},
    {0, 0, 0, 20, 20, 0, 0, 0},
    {5, -5, -10, 0, 0, -10, -5, 5},
    {5, 10, 10, -20, -20, 10, 10, 5},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

const int knightTable[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0, 0, 0, 0, -20, -40},
    {-30, 0, 10, 15, 15, 10, 0, -30},
    {-30, 5, 15, 20, 20, 15, 5, -30},
    {-30, 0, 15, 20, 20, 15, 0, -30},
    {-30, 5, 10, 15, 15, 10, 5, -30},
    {-40, -20, 0, 5, 5, 0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}
};

const int bishopTable[8][8] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 10, 10, 5, 0, -10},
    {-10, 5, 5, 10, 10, 5, 5, -10},
    {-10, 0, 10, 10, 10, 10, 0, -10},
    {-10, 10, 10, 10, 10, 10, 0, -10},
    {-10, 5, 0, 0, 0, 0, 5, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20}
};

const int rookTable[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {5, 10, 10, 10, 10, 10, 10, 5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {0, 0, 0, 5, 5, 0, 0, 0}
};

const int queenTable[8][8] = {
    {-20, -10, -10, -5, -5, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 5, 5, 5, 0, -10},
    {-5, 0, 5, 5, 5, 5, 0, -5},
    {0, 0, 5, 5, 5, 5, 0, -5},
    {-10, 5, 5, 5, 5, 5, 0, -10},
    {-10, 0, 5, 0, 0, 0, 0, -10},
    {-20, -10, -10, -5, -5, -10, -10, -20}
};

const int kingTable[8][8] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    {20, 20, 0, 0, 0, 0, 20, 20},
    {20, 30, 10, 0, 0, 10, 30, 20}
};


int max(int a, int b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}

int min(int a, int b) {
    if (a < b) {
        return a;
    }
    else {
        return b;
    }
}

void copyBoard(Board *dest, Board *src) {
    for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
            for (int i = 0; i < 3; i++) {
                dest->board[row][col][i] = src->board[row][col][i];
            }
        }
    }
    dest->turn = src->turn;
    dest->wkcastle = src->wkcastle;
    dest->wqcastle = src->wqcastle;
    dest->bkcastle = src->bkcastle;
    dest->bqcastle = src->bqcastle;
    dest->numMoves = src->numMoves;
    for (int i = 0; i < src->numMoves; i++) {
        for (int j = 0; j < 4; j++) {
            dest->moves[i][j] = src->moves[i][j];
        }
    }
}

void printMoves(Board *board) {
    for (int move = 0; move < board->numMoves; move++) {
        printf("%c%i%c%i, ", board->moves[move][0] + 97, board->moves[move][1] + 1, board->moves[move][2] + 97, board->moves[move][3] + 1);
    }
    printf("\n");
}

void printBoard(Board *board) {
    char piece[3];
    printf("\033[H\033[J");
    for (int row = 7; row >= 0; row--) {
        printf("\n%i|", row + 1);
        for (int column = 0; column < 8; column++) {
            strcpy(piece, board->board[row][column]);
            if (strcmp(piece, "wR") == 0) {
                printf("\u265C");
            }
            else if (strcmp(piece, "wN") == 0) {
                printf("\u265E");
            }
            else if (strcmp(piece, "wB") == 0) {
                printf("\u265D");
            }
            else if (strcmp(piece, "wQ") == 0) {
                printf("\u265B");
            }
            else if (strcmp(piece, "wK") == 0) {
                printf("\u265A");
            }
            else if (strcmp(piece, "wP") == 0) {
                printf("\u265F");
            }
            else if (strcmp(piece, "bR") == 0) {
                printf("\u2656");
            }
            else if (strcmp(piece, "bN") == 0) {
                printf("\u2658");
            }
            else if (strcmp(piece, "bB") == 0) {
                printf("\u2657");
            }
            else if (strcmp(piece, "bQ") == 0) {
                printf("\u2655");
            }
            else if (strcmp(piece, "bK") == 0) {
                printf("\u2654");
            }
            else if (strcmp(piece, "bP") == 0) {
                printf("\u2659");
            }  
            else {
                printf("\u25A1");
            }
            printf("|");
        }
    }
    printf("\n  a b c d e f g h\n");
}

int valid(int column, int row, int target_column, int target_row, Board *board) {
    if (target_row > 7 || target_column > 7 || target_row < 0 || target_column < 0) {return 0;}
    if (row == target_row && column == target_column) {return 0;}
    char piece[3];
    char turn = board->turn;
    if (board->board[target_row][target_column][0] == turn) {return 0;}
    strcpy(piece, board->board[row][column]);
    if (piece[0] != turn) {return 0;}
    if (piece[1] == 'R') {
        if (row == target_row) {
            if (target_column > column) {
                for (int i = column + 1; i < target_column; i++) {
                    if (board->board[row][i][0] != '-') {return 0;}
                }
                return 1;
            }
            else if  (target_column < column) {
                for (int i = column - 1; i > target_column; i--) {
                    if (board->board[row][i][0] != '-') {return 0;}
                }
                return 1;
            }
            else {return 0;}
        }
        else if (column == target_column) {
            if (target_row > row) {
                for (int i = row + 1; i < target_row; i++) {
                    if (board->board[i][column][0] != '-') {return 0;}
                }
                return 1;
            }
            else if (target_row < row) {
                for (int i = row - 1; i > target_row; i--) {
                    if (board->board[i][column][0] != '-') {return 0;}
                }
                return 1;
            }
            else {return 0;}
        }
        else {return 0;}
    }
    else if (piece[1] == 'N') {
        if (abs(row - target_row) + abs(column - target_column) == 3 && column != target_column && row != target_row) {return 1;}
        else {return 0;}
    }
    else if (piece[1] == 'B') {
        if (abs(target_row - row) == abs(target_column - column)) {
            if (target_column > column) {
                if (target_row > row) {
                    for (int i = 1; i < target_column - column; i++) {
                        if (board->board[row + i][column + i][0] != '-') {return 0;}
                    }
                    return 1;
                }
                else if (target_row < row) {
                    for (int i = 1; i < target_column - column; i++) {
                        if (board->board[row - i][column + i][0] != '-') {return 0;}
                    }
                    return 1;
                }
            }
            else if (target_column < column) {
                if (target_row > row) {
                    for (int i = 1; i < column - target_column; i++) {
                        if (board->board[row + i][column - i][0] != '-') {return 0;}
                    }
                    return 1;
                }
                else if (target_row < row) {
                    for (int i = 1; i < column - target_column; i++) {
                        if (board->board[row - i][column - i][0] != '-') {return 0;}
                    }
                    return 1;
                }
            }
            else {return 0;}
        }
    }
    else if (piece[1] == 'Q') {
        if (row == target_row) {
            if (target_column > column) {
                for (int i = column + 1; i < target_column; i++) {
                    if (board->board[row][i][0] != '-') {return 0;}
                }
                return 1;
            }
            else if  (target_column < column) {
                for (int i = column - 1; i > target_column; i--) {
                    if (board->board[row][i][0] != '-') {return 0;}
                }
                return 1;
            }
            else {return 0;}
        }
        else if (column == target_column) {
            if (target_row > row) {
                for (int i = row + 1; i < target_row; i++) {
                    if (board->board[i][column][0] != '-') {return 0;}
                }
                return 1;
            }
            else if (target_row < row) {
                for (int i = row - 1; i > target_row; i--) {
                    if (board->board[i][column][0] != '-') {return 0;}
                }
                return 1;
            }
            else {return 0;}
        }
        else if (abs(target_row - row) == abs(target_column - column)) {
            if (target_column > column) {
                if (target_row > row) {
                    for (int i = 1; i < target_column - column; i++) {
                        if (board->board[row + i][column + i][0] != '-') {return 0;}
                    }
                    return 1;
                }
                else if (target_row < row) {
                    for (int i = 1; i < target_column - column; i++) {
                        if (board->board[row - i][column + i][0] != '-') {return 0;}
                    }
                    return 1;
                }
            }
            else if (target_column < column) {
                if (target_row > row) {
                    for (int i = 1; i < column - target_column; i++) {
                        if (board->board[row + i][column - i][0] != '-') {return 0;}
                    }
                    return 1;
                }
                else if (target_row < row) {
                    for (int i = 1; i < column - target_column; i++) {
                        if (board->board[row - i][column - i][0] != '-') {return 0;}
                    }
                    return 1;
                }
            }
            else {return 0;}
        }
        else {return 0;}
    }
    else if (piece[1] == 'K') {
        if (abs(row - target_row) <= 1 && abs(column - target_column) <= 1) {
            return 1;
        }
        else if (row == target_row && abs(column - target_column) == 2) {
            if (board->turn == 'w' && row == 0) {
                if (target_column == 6 && board->wkcastle) {
                    if (board->board[0][5][0] == '-' && board->board[0][6][0] == '-') {return 1;}
                    else {return 0;}
                }
                else if(target_column == 2 && board->wqcastle) {
                    if (board->board[0][3][0] == '-' && board->board[0][2][0] == '-' && board->board[0][1][0] == '-') {return 1;}
                    else {return 0;}
                }
                else {return 0;}
            }
            else if (board->turn == 'b' && row == 7) {
                if (target_column == 6 && board->bkcastle) {
                    if (board->board[7][5][0] == '-' && board->board[7][6][0] == '-') {return 1;}
                    else {return 0;}
                }
                else if (target_column == 2 && board->bqcastle) {
                    if (board->board[7][3][0] == '-' && board->board[7][2][0] == '-' && board->board[7][1][0] == '-') {return 1;}
                    else {return 0;}
                }
                else {return 0;}
            }
            else {return 0;}
        }
        else {return 0;}
    }
    else if (piece[1] == 'P') {
        if (piece[0] == 'w') {
            if (target_row - row == 1) {
                if (column == target_column && board->board[target_row][target_column][0] == '-') {return 1;}
                else if (abs(column - target_column) == 1 && (board->board[target_row][target_column][0] != '-' || (board->passant && target_row == board->passant_row && target_column == board->passant_col))) {return 1;}
                else {return 0;}
            }
            else if (target_row - row == 2 && row == 1 && column == target_column && board->board[target_row][target_column][0] == '-' && board->board[target_row - 1][target_column][0] == '-') {
                return 1;
            }
            else {return 0;}
        }
        else if (piece[0] == 'b') {
            if (row - target_row == 1) {
                if (column == target_column && board->board[target_row][target_column][0] == '-') {return 1;}
                else if (abs(column - target_column) == 1 && (board->board[target_row][target_column][0] != '-' || (board->passant && target_row == board->passant_row && target_column == board->passant_col))) {return 1;}
                else {return 0;}
            }
            else if (row - target_row == 2 && row == 6 && column == target_column && board->board[target_row][target_column][0] == '-' && board->board[target_row + 1][target_column][0] == '-') {
                return 1;
            }
            else {return 0;}
        }
    }
    else {return 0;}
    return 0;
}

long evaluate(char turn, Board *board) {
    long score = 0;
    int r;

    for (int column = 0; column < 8; column++) {
        for (int row = 0; row < 8; row++) {
            char piece = board->board[row][column][1];
            int value = 0;
            r = (board->board[row][column][0] == 'b') ? row : 7 - row;
            switch (piece) {
                case 'P': value = pawnValue + pawnTable[r][column]; break;
                case 'N': value = knightValue + knightTable[r][column]; break;
                case 'B': value = bishopValue + bishopTable[r][column]; break;
                case 'R': value = rookValue + rookTable[r][column]; break;
                case 'Q': value = queenValue + queenTable[r][column]; break;
                case 'K': value = kingValue + kingTable[r][column]; break;
            }
            score += (board->board[row][column][0] == turn) ? value : -value;
        }
    }

    score += board->numMoves;

    score += board->bcastled*100;
    score -= 100*((board->bkcastle == 0) && (board->bqcastle == 0) && (board->bcastled == 0));


    return score;
}

void validMoves(Board *board) {
    char moves[256][4];
    int numMoves = 0;
    char piece;
    char color;
    for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
            piece = board->board[row][col][1];
            color = board->board[row][col][0];
            if (piece == 'P' && color == 'w') {
                for (int t_row = row + 1; t_row < row + 3; t_row++) {
                    for (int t_col = col - 1; t_col < col + 2; t_col++) {
                        if (valid(col, row, t_col, t_row, board)) {
                            moves[numMoves][0] = col;
                            moves[numMoves][1] = row;
                            moves[numMoves][2] = t_col;
                            moves[numMoves][3] = t_row;
                            numMoves++;
                        }
                    }
                }
            }
            else if (piece == 'P' && color == 'b') {
                for (int t_row = row - 1; row - t_row < 3; t_row--) {
                    for (int t_col = col -1; t_col < col + 2; t_col++) {
                        if (valid(col, row, t_col, t_row, board)) {
                            moves[numMoves][0] = col;
                            moves[numMoves][1] = row;
                            moves[numMoves][2] = t_col;
                            moves[numMoves][3] = t_row;
                            numMoves++;
                        }
                    }
                }
            }
            else if (piece == 'R') {
                for (int dist = 0; dist < 8; dist++) {
                    if (valid(col, row, dist, row, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = dist;
                        moves[numMoves][3] = row;
                        numMoves++;
                    }
                    if (valid(col, row, col, dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col;
                        moves[numMoves][3] = dist;
                        numMoves++;
                    }
                }
            }
            else if (piece == 'N') {
                if (valid(col, row, col + 2, row + 1, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col + 2;
                    moves[numMoves][3] = row + 1;
                    numMoves++;
                }
                if (valid(col, row, col + 2, row - 1, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col + 2;
                    moves[numMoves][3] = row - 1;
                    numMoves++;
                }
                if (valid(col, row, col - 2, row + 1, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col - 2;
                    moves[numMoves][3] = row + 1;
                    numMoves++;
                }
                if (valid(col, row, col - 2, row - 1, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col - 2;
                    moves[numMoves][3] = row - 1;
                    numMoves++;
                }
                if (valid(col, row, col + 1, row + 2, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col + 1;
                    moves[numMoves][3] = row + 2;
                    numMoves++;
                }
                if (valid(col, row, col + 1, row - 2, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col + 1;
                    moves[numMoves][3] = row - 2;
                    numMoves++;
                }
                if (valid(col, row, col - 1, row + 2, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col - 1;
                    moves[numMoves][3] = row + 2;
                    numMoves++;
                }
                if (valid(col, row, col - 1, row - 2, board)) {
                    moves[numMoves][0] = col;
                    moves[numMoves][1] = row;
                    moves[numMoves][2] = col - 1;
                    moves[numMoves][3] = row - 2;
                    numMoves++;
                }
            }
            else if (piece == 'B') {
                for (int dist = 0; dist < 8; dist++) {
                    if (valid(col, row, col + dist, row + dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col + dist;
                        moves[numMoves][3] = row + dist;
                        numMoves++;
                    }
                    if (valid(col, row, col + dist, row - dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col + dist;
                        moves[numMoves][3] = row - dist;
                        numMoves++;
                    }
                    if (valid(col, row, col - dist, row + dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col - dist;
                        moves[numMoves][3] = row + dist;
                        numMoves++;
                    }
                    if (valid(col, row, col - dist, row - dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col - dist;
                        moves[numMoves][3] = row - dist;
                        numMoves++;
                    }
                }
            }
            else if (piece == 'Q') {
                for (int dist = 0; dist < 8; dist++) {
                    if (valid(col, row, dist, row, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = dist;
                        moves[numMoves][3] = row;
                        numMoves++;
                    }
                    if (valid(col, row, col, dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col;
                        moves[numMoves][3] = dist;
                        numMoves++;
                    }
                    if (valid(col, row, col + dist, row + dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col + dist;
                        moves[numMoves][3] = row + dist;
                        numMoves++;
                    }
                    if (valid(col, row, col + dist, row - dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col + dist;
                        moves[numMoves][3] = row - dist;
                        numMoves++;
                    }
                    if (valid(col, row, col - dist, row + dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col - dist;
                        moves[numMoves][3] = row + dist;
                        numMoves++;
                    }
                    if (valid(col, row, col - dist, row - dist, board)) {
                        moves[numMoves][0] = col;
                        moves[numMoves][1] = row;
                        moves[numMoves][2] = col - dist;
                        moves[numMoves][3] = row - dist;
                        numMoves++;
                    }
                }
            }
            else if (piece == 'K') {
                for (int hDist = -2; hDist < 3; hDist++) {
                    for (int vDist = -1; vDist < 2; vDist++) {
                        if (valid(col, row, col + hDist, row + vDist, board)) {
                            moves[numMoves][0] = col;
                            moves[numMoves][1] = row;
                            moves[numMoves][2] = col + hDist;
                            moves[numMoves][3] = row + vDist;
                            numMoves++;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < numMoves; i++) {
        for (int j = 0; j < 4; j++) {
            board->moves[i][j] = moves[i][j];
        }
    }
    board->numMoves = numMoves;
}

void initBoard(Board *board) {
    board->turn = 'w';
    board->wkcastle = 1;
    board->wqcastle = 1;
    board->bkcastle = 1;
    board->bqcastle = 1;
    board->bcastled = 0;
    board->wcastled = 0;
    board->passant = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (row == 0 || row == 7) {
                if (col == 0 || col == 7) {
                    board->board[row][col][1] = 'R';
                    }
                else if (col == 1 || col == 6) {
                    board->board[row][col][1] = 'N';
                    }
                else if (col == 2 || col == 5) {
                    board->board[row][col][1] = 'B';
                }
                else if (col == 3) {
                    board->board[row][col][1] = 'Q';
                }
                else if (col == 4) {
                    board->board[row][col][1] = 'K';
                }
            }
            else if (row == 1 || row == 6) {
                board->board[row][col][1] = 'P';
            }
            if (row < 2) {
                board->board[row][col][0] = 'w';
                }
            else if (row > 5) {
                board->board[row][col][0] = 'b';
            }
            else {
                board->board[row][col][0] = '-';
                board->board[row][col][1] = '-';
            }
            board->board[row][col][2] = '\0';
        }
    }
    validMoves(board);
}

void makeMove(int column, int row, int target_column, int target_row, Board *board) {
    if (board->board[row][column][1] == 'P' && (column - target_column) && board->board[target_row][target_column][0] == '-') {
        board->board[row][target_column][0] = '-';
        board->board[row][target_column][1] = '-';
    }
    strcpy(board->board[target_row][target_column], board->board[row][column]);
    if (board->board[row][column][1] == 'P' && target_row - row == 2) {
        board->passant = 1;
        board->passant_row = row + 1;
        board->passant_col = column;
    }
    else if (board->board[row][column][1] == 'P' && row - target_row == 2) {
        board->passant = 1;
        board->passant_row = row - 1;
        board->passant_col = column;
    }
    else {
        board->passant = 0;
    }
    if (board->board[row][column][1] == 'P' && (target_row == 7 || target_row == 0)) {
        board->board[target_row][target_column][1] = 'Q';
    }
    strcpy(board->board[row][column], "--");
    if (board->board[target_row][target_column][1] == 'K' && abs(column - target_column) == 2) {
        if (target_column > column) {
            board->board[row][column + 1][0] = board->turn;
            board->board[row][column + 1][1] = 'R';
            board->board[row][column + 3][0] = '-';
            board->board[row][column + 3][1] = '-';
        }
        else if (target_column < column) {
            board->board[row][column - 1][0] = board->turn;
            board->board[row][column - 1][1] = 'R';
            board->board[row][column - 3][0] = '-';
            board->board[row][column - 3][1] = '-';
            board->board[row][column - 4][0] = '-';
            board->board[row][column - 4][1] = '-';
        }
        if (board->turn == 'w') {
            board->wkcastle = 0;
            board->wqcastle = 0;
            board->wcastled = 1;
        }
        else {
            board->bkcastle = 0;
            board->bqcastle = 0;
            board->bcastled = 1;
      }
    }
    if (row == 0 && column == 4) {
      board->wkcastle = 0;
      board->wqcastle = 0;
    }
    else if (row == 7 && column == 4) {
      board->bkcastle = 0;
      board->bqcastle = 0;
    }
    if ((column == 0 && row == 0) || (target_column == 0 && target_row == 0)) {board->wqcastle = 0;}
    else if ((column == 7 && row == 0) || (target_column == 7 && target_row == 0)) {board->wkcastle = 0;}
    else if ((column == 0 && row == 7) || (target_column == 0 && target_row == 7)) {board->bqcastle = 0;}
    else if ((column == 7 && row == 7) || (column == 7 && row == 7)) {board->bkcastle = 0;}
    if (board->turn == 'w') {
      board->turn = 'b';
    }
    else {
      board->turn = 'w';
    }
    validMoves(board);
}

void playerMove(Board *board) {
    char move[10];
    printBoard(board);
    printMoves(board);
    while (1) {
        printf("Make a move: ");
        scanf("%s", move);
        if (valid(move[0] - 97, move[1] - 49, move[2] - 97, move[3] - 49, board)) {
            makeMove(move[0] - 97, move[1] - 49, move[2] - 97, move[3] - 49, board);
            break;
        }
    }
}

long minimax(char turn, Board *board, int depth, long alpha, long beta) {
    if (depth == 0) {
        return evaluate(turn, board);
    }
    long score = (turn == board->turn) ? -999999 : 999999;
    Board tempboard;
    for (int i = 0; i < board->numMoves; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        if (turn == board->turn) {
            score = max(score, minimax(turn, &tempboard, depth, alpha, beta));
            alpha = max(alpha, score);
        } else {
            score = min(score, minimax(turn, &tempboard, depth - 1, alpha, beta));
        }
    }
    return score;
}

void* function1(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = 0; i < (board->numMoves / 8); i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 1;
    return NULL;
}

void* function2(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = (board->numMoves / 8); i < ((2 * board->numMoves) / 8); i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 2;
    return NULL;
}

void* function3(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((2 * board->numMoves) / 8); i < ((3 * board->numMoves) / 8); i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 3;
    return NULL;
}

void* function4(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((3 * board->numMoves) / 8); i < (4 * board->numMoves) / 8; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 4;
    return NULL;
}

void* function5(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((4 * board->numMoves) / 8); i < (5 * board->numMoves) / 8; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 5;
    return NULL;
}

void* function6(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((5 * board->numMoves) / 8); i < (6 * board->numMoves) / 8; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 6;
    return NULL;
}

void* function7(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((6 * board->numMoves) / 8); i < (7 * board->numMoves) / 8; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 7;
    return NULL;
}

void* function8(void* arg) {
    thread_arg* args = (thread_arg*)arg;
    Board *board = args->board;
    int depth = args->depth;
    char turn = 'b';
    if (board->turn == 'w') {
        turn = 'w';
    }
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    long score;
    Board tempboard;
    long best_score = -999999;
    for (int i = ((7 * board->numMoves) / 8); i < (8 * board->numMoves) / 8; i++) {
        memcpy(&tempboard, board, sizeof(Board));
        makeMove(board->moves[i][0], board->moves[i][1], board->moves[i][2], board->moves[i][3], &tempboard);
        score = minimax(turn, &tempboard, depth, -999999, 999999);
        if (score > best_score) {
          col = board->moves[i][0];
          row = board->moves[i][1];
          t_col = board->moves[i][2];
          t_row = board->moves[i][3];
          best_score = score;
        }
        percentage = percentage + increment;
        printf("\r%.0f%c", (percentage / board->numMoves) * 100, '%');
    }
    args->result.move[0] = col; args->result.move[1] = row; args->result.move[2] = t_col; args->result.move[3] = t_row;
    args->result.score = best_score;
    args->function_number = 8;
    return NULL;
}

void engineMove(Board *board, int depth) {
    int col = 0;
    int row = 0;
    int t_col = 0;
    int t_row = 0;
    pthread_t thread1, thread2, thread3, thread4, thread5, thread6, thread7, thread8;
    thread_arg arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8;

    arg1.depth = depth;
    arg1.board = board;

    arg2.depth = depth;
    arg2.board = board;

    arg3.depth = depth;
    arg3.board = board;

    arg4.depth = depth;
    arg4.board = board;

    arg5.depth = depth;
    arg5.board = board;

    arg6.depth = depth;
    arg6.board = board;

    arg7.depth = depth;
    arg7.board = board;

    arg8.depth = depth;
    arg8.board = board;

    pthread_create(&thread1, NULL, function1, (void*)&arg1);
    pthread_create(&thread2, NULL, function2, (void*)&arg2);
    pthread_create(&thread3, NULL, function3, (void*)&arg3);
    pthread_create(&thread4, NULL, function4, (void*)&arg4);
    pthread_create(&thread5, NULL, function5, (void*)&arg5);
    pthread_create(&thread6, NULL, function6, (void*)&arg6);
    pthread_create(&thread7, NULL, function7, (void*)&arg7);
    pthread_create(&thread8, NULL, function8, (void*)&arg8);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);
    pthread_join(thread6, NULL);
    pthread_join(thread7, NULL);
    pthread_join(thread8, NULL);

    move a = arg1.result;
    move b = arg2.result;
    move c = arg3.result;
    move d = arg4.result;
    move e = arg5.result;
    move f = arg6.result;
    move g = arg7.result;
    move h = arg8.result;
    
    if (a.score > b.score && a.score > c.score && a.score > d.score && a.score > e.score && a.score > f.score && a.score > g.score && a.score > h.score) {
        col = a.move[0], row = a.move[1], t_col = a.move[2], t_row = a.move[3];
    }
    else if (b.score > a.score && b.score > c.score && b.score > d.score && b.score > e.score && b.score > f.score && b.score > g.score && b.score > h.score) {
        col = b.move[0], row = b.move[1], t_col = b.move[2], t_row = b.move[3];
    }
    else if (c.score > a.score && c.score > b.score && c.score > d.score && c.score > e.score && c.score > f.score && c.score > g.score && c.score > h.score) {
        col = c.move[0], row = c.move[1], t_col = c.move[2], t_row = c.move[3];
    }
    else if (d.score > a.score && d.score > b.score && d.score > c.score && d.score > e.score && d.score > f.score && d.score > g.score && d.score > h.score) {
        col = d.move[0], row = d.move[1], t_col = d.move[2], t_row = d.move[3];
    }
    else if (e.score > a.score && e.score > b.score && e.score > c.score && e.score > d.score && e.score > f.score && e.score > g.score && e.score > h.score) {
        col = e.move[0], row = e.move[1], t_col = e.move[2], t_row = e.move[3];
    }
    else if (f.score > a.score && f.score > b.score && f.score > c.score && f.score > d.score && f.score > e.score && f.score > g.score && f.score > h.score) {
        col = f.move[0], row = f.move[1], t_col = f.move[2], t_row = f.move[3];
    }
    else if (g.score > a.score && g.score > b.score && g.score > c.score && g.score > d.score && g.score > e.score && g.score > f.score && g.score > h.score) {
        col = g.move[0], row = g.move[1], t_col = g.move[2], t_row = g.move[3];
    }
    else {
        col = h.move[0], row = h.move[1], t_col = h.move[2], t_row = h.move[3];
    }
    percentage = 0;
    makeMove(col, row, t_col, t_row, board);
}