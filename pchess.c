#include "defs.h"

// chcp 65001

int main(void) {
    system("chcp 65001");
    Board board;
    initBoard(&board);
    while (1) {
        playerMove(&board);
        engineMove(&board, 2);
    }
}