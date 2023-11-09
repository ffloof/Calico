#include <vector>
#include <chrono>

// TODO: define constants for everything

int maxdepth = 0;
move bestMove;

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
        legals++;

        int score = -alphabeta(nextBoard, -beta, -alpha, depth-1);
        delete nextBoard;

        if (score > alpha) {
            alpha = score;
            if (score > bestScore) {
                bestScore = score;
                if (depth == maxdepth) bestMove = m;
            }
            if (score >= beta) {
                return score;
            }
        }
    }

    if (legals == 0) {
        if (b->inCheck) return -10000;
        return -69;
    }

    return bestScore;
}

move iterativeSearch(board* b, int timeAlloc) {
    auto t1 = std::chrono::high_resolution_clock::now();
    
    for(int depth=1;depth<20;depth++){
        maxdepth = depth;
        int score = alphabeta(b, -10000, 10000, depth);
        auto t2 = std::chrono::high_resolution_clock::now();
        int time = (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();
        std::cout << "info depth " << depth << " cp " << score << " time " << time << std::endl;
        if (time > timeAlloc) break;
    }

    return bestMove;
}
