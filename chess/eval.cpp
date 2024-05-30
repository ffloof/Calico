#include "misc.cpp"
#include "nnue.cpp"

int standard[120] = {
    64, -1, -1, -1, -1, -1, -1, -1, -1, -1,
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

char reconvert[14] = {
    '/', 'K', 'Q', 'R', 'B', 'N', 'P', 'k', 'q', 'r', 'b', 'n', 'p', '/',
};

void initNNUE(){
    nnue_init("nn-04cf2b4ed1da.nnue");
    //std::cout << nnue_evaluate_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1") << std::endl;
}

NNUEdata nn_stack[256];
NNUEdata* stack[3];

int proxynnue(int player, int* pieces, int* squares, int ply){
    nn_stack[ply].accumulator.computedAccumulation = 0;
    
    //fill the stack with current ply and previous 2 plies' networks
    stack[0] = &nn_stack[ply];
    stack[1] = (ply > 1) ? &nn_stack[ply - 1] : 0;
    stack[2] = (ply > 2) ? &nn_stack[ply - 2] : 0;
    int score = nnue_evaluate_incremental(player, pieces, squares, stack);

    //std::cout << ply << "=" << score << std::endl;

    return score;
} 


int pieces[33];
int squares[33];

int evaluate(board* b){
    // b->print();
    
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
    }

    pieces[i] = 0;

    int score = proxynnue(!b->whiteToMove, pieces, squares, b->ply);

    return score;
}

void board::updateEval(int from, int to, int8_t piece, int changeIndex=0){
    DirtyPiece* dp = &(nn_stack[ply].dirtyPiece);
    dp->dirtyNum = changeIndex + 1;
    dp->pc[changeIndex]    = convert[piece];
    dp->from[changeIndex]  = standard[from];
    dp->to[changeIndex]    = standard[to];
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