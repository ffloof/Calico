#include <vector>
#include <chrono>

// TODO: define constants for mate

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
            if (score >= beta) break;
        }
    }

    /* TODO: figure out how to handle check in qsearch
    if (b->inCheck) return -10000;
    return -69;
    */
    
    return alpha;
}

struct searcher {
    int nodes;
    int ply;
    unsigned long long repetition[255];
    double timeAlloc;

    // TODO: add evals here
    // TODO: add history heuristic here

    void push(unsigned long long hash){
        repetition[ply] = hash;
        nodes++;
        ply += 1;
    }

    void pop(){
        ply -= 1;
    }

    bool isRepetition(unsigned long long hash){
        for(int i=0;i<ply;i++){
            if (repetition[i] == hash) return true;
        }
        return false;
    }

    int alphabeta(board* b, int alpha, int beta, int depth){
        if (depth <= 0) {
            return qsearch(b, alpha, beta);
        }

        unsigned long long hash = b->getHash();
        if (isRepetition(hash) && ply != 0) return -16;
        push(hash);

        std::vector<move> moves = b->GeneratesMoves();
        std::vector<int> priorities(moves.size());

        ttentry* tentry = tableget(hash);
        move tablemove;
        if (tentry != nullptr) {
            tablemove = tentry->tableMove;
            if (depth <= tentry->depth && ply != 0){ 
                if ((tentry->bound == EXACT)
                || (tentry->bound == LOWERBOUND && tentry->score >= beta) 
                || (tentry->bound == UPPERBOUND && tentry->score <= alpha)) { 
                    pop();
                    return tentry->score;
                }
            }
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
                    if (score >= beta) break;
                }
            }
        }

        int8_t boundtype = EXACT;
        if (bestScore < alpha) boundtype = UPPERBOUND; // TODO: not exactly correct we want to know if we raised alpha
        if (bestScore >= beta) boundtype = LOWERBOUND;

        tableset(b, bestMove, depth, bestScore, boundtype);
        if (legals == 0) {
            pop();
            if (b->inCheck) return -10000 + ply;
            return -16;
        }

        pop();
        return bestScore;
    }
};

move iterativeSearch(board* b, int timeAlloc) {
    auto t1 = std::chrono::high_resolution_clock::now();
    searcher s = searcher{};
    
    for(int depth=1;depth<20;depth++){
        int score = s.alphabeta(b, -10000, 10000, depth);
        auto t2 = std::chrono::high_resolution_clock::now();
        int time = (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();
        std::cout << "info depth " << depth << " cp " << score << " time " << time << " nodes " << s.nodes << " ";
        printpv(b);
        std::cout << std::endl;
        std::cout << "ply" << s.ply << std::endl;
        if (time > timeAlloc) break;
    }

    ttentry* tentry = tableget(b->getHash());
    return tentry->tableMove;
}
