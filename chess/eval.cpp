#include "misc.cpp"
#include "nnue.cpp"

int standard[120] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
    -1, 48, 49, 50, 51, 52, 53, 54, 55, -1,
    -1, 40, 41, 42, 43, 44, 45, 46, 47, -1,
    -1, 32, 33, 34, 35, 36, 37, 38, 39, -1,
    -1, 24, 25, 26, 27, 28, 29, 30, 31, -1,
    -1, 16, 17, 18, 19, 20, 21, 22, 23, -1,
    -1,  8,  9, 10, 11, 12, 13, 14, 15, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

int convert[14] = {
    0, 0, 12, 6, 11, 5, 10, 4, 9, 3, 8, 2, 7, 1, 
};

void initNNUE(){
    nnue_init("nn-04cf2b4ed1da.nnue");
    nnue_evaluate_fen("rnb1kbnr/pppp1ppp/8/4P3/7q/8/PPPPP1PP/RNBQKBNR w KQkq - 1 3");
}

int proxynnue(int player, int* pieces, int* squares){
    NNUEdata nnue;
    nnue.accumulator.computedAccumulation = 0;

    Position pos;
    pos.nnue[0] = &nnue;
    pos.nnue[1] = 0;
    pos.nnue[2] = 0;
    pos.player = player;
    pos.pieces = pieces;
    pos.squares = squares;
    
    int score = nnue_evaluate_pos(&pos);
    return score;
}

int evaluate(board* b){
    int *pieces = new int[33];
    int *squares = new int[33];
    
    pieces[0] = 1;
    pieces[1] = 7;
    squares[0] = standard[b->kings[1]];
    squares[1] = standard[b->kings[0]];

    int i = 2;
    for (int sq = A8;sq<=H1;sq++) {
        if (b->squares[sq] <= BORDER || b->squares[sq] >= KING) continue;
        
        pieces[i] = convert[b->squares[sq]];
        squares[i] = standard[sq];
        i++;
        if (i > 33) {
            std::cout << "NOT NORMAL";
        }
    }

    pieces[i] = 0;

    int score = proxynnue(!b->whiteToMove, pieces, squares);

    delete[] pieces;
    delete[] squares;
    return score;
}




int mailbox[64] = {
    21, 22, 23, 24, 25, 26, 27, 28,
    31, 32, 33, 34, 35, 36, 37, 38,
    41, 42, 43, 44, 45, 46, 47, 48,
    51, 52, 53, 54, 55, 56, 57, 58,
    61, 62, 63, 64, 65, 66, 67, 68,
    71, 72, 73, 74, 75, 76, 77, 78,
    81, 82, 83, 84, 85, 86, 87, 88,
    91, 92, 93, 94, 95, 96, 97, 98
};



int phases[14] = {0,0,0,0,2,2,2,2,3,3,8,8,0};