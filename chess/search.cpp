#include <vector>

// TODO: define constants for everything

int alphabeta(board* b, int alpha, int beta, int depth){
    if (depth <= 0) {
        return evaluate(b);
    }

    std::vector<move> moves = b->GeneratesMoves();

    int legals = 0;
    int bestScore = -10000;

    for (move m : moves) {
        board* nextBoard = apply(b,m);
        if (nextBoard == nullptr) continue;

        int score = -alphabeta(nextBoard, -beta, -alpha, depth-1);
        delete nextBoard;

        if (score > alpha) {
            alpha = score;
            if (score > bestScore) {
                bestScore = score;
            }
            if (score >= beta) {
                return score;
            }
        }
    }

    if (legals == 0) {
        if (b->inCheck) return -10000;
        return -20;
    }

    return bestScore;
}

move iterativeSearch(board* b, int timeAlloc) {

}
