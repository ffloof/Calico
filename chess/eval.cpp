#define NNUE_EMBEDDED
#define NNUE_FILE "nn-04cf2b4ed1da.nnue"
#include "misc.cpp"
#include "nnue.cpp"

int standard[128] = {
    
    56, 57, 58, 59, 60, 61, 62, 63, 64, 64, 64, 64, 64, 64, 64, 64, 
    48, 49, 50, 51, 52, 53, 54, 55, 64, 64, 64, 64, 64, 64, 64, 64, 
    40, 41, 42, 43, 44, 45, 46, 47, 64, 64, 64, 64, 64, 64, 64, 64, 
    32, 33, 34, 35, 36, 37, 38, 39, 64, 64, 64, 64, 64, 64, 64, 64, 
    24, 25, 26, 27, 28, 29, 30, 31, 64, 64, 64, 64, 64, 64, 64, 64, 
    16, 17, 18, 19, 20, 21, 22, 23, 64, 64, 64, 64, 64, 64, 64, 64, 
     8,  9, 10, 11, 12, 13, 14, 15, 64, 64, 64, 64, 64, 64, 64, 64, 
     0,  1,  2,  3,  4,  5,  6,  7, 64, 64, 64, 64, 64, 64, 64, 64, 
    
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

int NNUEpieces[33];
int NNUEsquares[33];

int proxynnue(int player, int* pieces, int* squares, int ply){
    nn_stack[ply].accumulator.computedAccumulation = 0;
    
    //fill the stack with current ply and previous 2 plies' networks
    stack[0] = &nn_stack[ply];
    stack[1] = (ply > 1) ? &nn_stack[ply - 1] : 0;
    stack[2] = (ply > 2) ? &nn_stack[ply - 2] : 0;
    int score = nnue_evaluate_incremental(player, NNUEpieces, NNUEsquares, stack);

    //std::cout << ply << "=" << score << std::endl;

    return score;
} 




int evaluate(board* b){
    NNUEpieces[0] = KING;
    NNUEpieces[1] = KING + BLACK;
    NNUEsquares[0] = standard[b->kings[1]];
    NNUEsquares[1] = standard[b->kings[0]];

    int i = 2;
    for (int sq = A8;sq<=H1;sq++) {
        if (b->squares[sq] == EMPTY || b->squares[sq] == KING || b->squares[sq] == KING + BLACK) continue;
        
        NNUEpieces[i] = b->squares[sq];
        NNUEsquares[i] = standard[sq];

        i++;
    }

    NNUEpieces[i] = 0;

    int score = proxynnue(!b->whiteToMove, NNUEpieces, NNUEsquares, b->ply);

    return score;
}

void board::updateEval(int from, int to, int8_t piece, int changeIndex=0){
    DirtyPiece* dp = &(nn_stack[ply].dirtyPiece);
    dp->dirtyNum = changeIndex + 1;
    dp->pc[changeIndex]    = piece;
    dp->from[changeIndex]  = standard[from];
    dp->to[changeIndex]    = standard[to];
}
