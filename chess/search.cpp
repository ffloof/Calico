#include <vector>
#include <chrono>

// TODO: define constants for everything

int qsearch(board* b, int alpha, int beta) {
    int standpat = evaluate(b);
    if (standpat > alpha) {
        alpha = standpat;
        if (alpha >= beta) return alpha;
    }
    
    std::vector<move> moves = b->GeneratesMoves(true);
    std::vector<int> priorities(moves.size());

    for (int i=0;i<moves.size();i++){
        move m = moves[i];
        priorities[i] = (abs(b->squares[m.end]) * 8) - abs(b->squares[m.start]) + 10;
    }

    int legals = 0;
    int bestScore = -10000;

    for (int i=0;i<moves.size();i++){
        int bestPriority = 0;
        int bestIndex = 0;
        for (int j=i;j<moves.size();j++){
            if (priorities[j] > bestPriority) {
                bestPriority = priorities[j];
                bestIndex = j;
            }
        }

        move m = moves[bestIndex];
        std::swap(moves[i],moves[bestIndex]);
        std::swap(priorities[i],priorities[bestIndex]);

        board* nextBoard = apply(b,m);
        if (nextBoard == nullptr) continue;
        legals++;

        int score = -qsearch(nextBoard, -beta, -alpha);
        delete nextBoard;

        if (score > alpha) {
            alpha = score;
            if (score >= beta) {
                return score;
            }
        }
    }

    /* TODO: figure out how to handle check in qsearch
    if (b->inCheck) return -10000;
    return -69;
    */
    
    return alpha;
}


int maxdepth = 0;

int alphabeta(board* b, int alpha, int beta, int depth){
    if (depth <= 0) {
        return qsearch(b, alpha, beta);
    }

    std::vector<move> moves = b->GeneratesMoves();
    std::vector<int> priorities(moves.size());

    ttentry* tentry = tableget(b);
    move tablemove;
    if (tentry != nullptr) {
        tablemove = tentry->tableMove;
    }

    for (int i=0;i<moves.size();i++){
        move m = moves[i];
        priorities[i] = (abs(b->squares[m.end]) * 8) - abs(b->squares[m.start]) + 10;
        if(m.start == tablemove.start && m.end == tablemove.end && m.flag == tablemove.flag) priorities[i] = 1000;
    }

    int legals = 0;
    int bestScore = -10000;
    move bestMove;

    for (int i=0;i<moves.size();i++){
        int bestPriority = 0;
        int bestIndex = 0;
        for (int j=i;j<moves.size();j++){
            if (priorities[j] > bestPriority) {
                bestPriority = priorities[j];
                bestIndex = j;
            }
        }

        move m = moves[bestIndex];
        std::swap(moves[i],moves[bestIndex]);
        std::swap(priorities[i],priorities[bestIndex]);

        board* nextBoard = apply(b,m);
        if (nextBoard == nullptr) continue;
        legals++;

        int score = -alphabeta(nextBoard, -beta, -alpha, depth-1);
        delete nextBoard;

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            if (score > alpha) {
                alpha = score;
                
                if (score >= beta) {
                    tableset(b, bestMove, depth, bestScore, 2);
                    return score;
                }
            }
        }
    }

    if (legals == 0) {
        if (b->inCheck) return -10000;
        return -20;
    }

    tableset(b, bestMove, depth, bestScore, 2);
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

    ttentry* tentry = tableget(b);
    return tentry->tableMove;
}
