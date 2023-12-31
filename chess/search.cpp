#include <vector>
#include <chrono>

const int MATE_SCORE = 10000;
const int DRAW_SCORE = 0;

const int TABLEMOVE_PRIORITY = 1000000000;
const int CAPTURE_PRIORITY   = 100000000;

struct searcher {
    int nodes;
    int ply;
    std::vector<unsigned long long> prev;
    unsigned long long repetition[255];
    int64_t history[2][128][128];
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    int timeAlloc;


    // TODO: add evals here

    void push(unsigned long long hash){
        repetition[ply] = hash;
        nodes++;
        ply += 1;
    }

    void pop(){
        ply -= 1;
    }

    bool isRepetition(unsigned long long hash){
        if (ply == 0) return false;

        for(int i=0;i<ply;i++){
            if (repetition[i] == hash) return true;
        }

        for (int i=1;i<prev.size();i+=2) {
            if (prev[i] == hash) return true; 
        }

        return false;
    }

    int ellapsedTime(){
        auto currentTime = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    }

    bool outOfTime(){
        return ellapsedTime() > timeAlloc;
    }

    int qsearch(board* b, int alpha, int beta) {
        ttentry* tentry = tableget(b->getHash());
        move tablemove;
        if (tentry != nullptr) {
            tablemove = tentry->tableMove;
            if ((tentry->bound == EXACT)
            || (tentry->bound == LOWERBOUND && tentry->score >= beta) 
            || (tentry->bound == UPPERBOUND && tentry->score <= alpha)) { 
                return tentry->score;
            }   
        }


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
            nodes++;

            int score = -qsearch(nextBoard, -beta, -alpha);
            delete nextBoard;

            if (score > alpha) {
                alpha = score;
                if (score >= beta) break;
            }
        }

        //if (b->inCheck) return alpha - 50;
        
        return alpha;
    }

    int alphabeta(board* b, int alpha, int beta, int depth){
        if (depth <= 0) {
            return qsearch(b, alpha, beta);
        }

        bool pv = beta > alpha + 1;

        unsigned long long hash = b->getHash();
        if (isRepetition(hash)) return DRAW_SCORE;
        push(hash);

        ttentry* tentry = tableget(hash);

        // TODO: could this be leading us astray in endgame positions?
        // Internal iterative deepening
        if (tentry == nullptr && depth > 4) {
            alphabeta(b, alpha, beta, depth/2);
            tentry = tableget(hash);
        }

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
        
        
        int eval = evaluate(b);

        // Null move pruning
        if (!pv && depth >= 2 && eval >= beta && b->phase > 8) {
            board* nmBoard = apply(b, NULLMOVE);
            if (nmBoard != nullptr) {
                int nmReduction = 3 + (depth/3);
                int nmScore = -alphabeta(nmBoard, -beta, -alpha, depth - nmReduction);
                delete nmBoard;
                if (nmScore >= beta) {
                    pop();
                    return nmScore;
                }
            }
        }

        std::vector<move> moves = b->GeneratesMoves();
        std::vector<int> priorities(moves.size());

        // Reverse futility pruning
        if (!pv && !b->inCheck && depth < 5 && eval >= beta + (125*depth)) {
            pop();
            return eval;
        }

        /*
        // Check extension
        if (b->inCheck) depth += 1;
        */

        for (int i=0;i<moves.size();i++){
            move m = moves[i];
            priorities[i] = (abs(b->squares[m.end]) * 8) - abs(b->squares[m.start]) + CAPTURE_PRIORITY;
            priorities[i] += history[b->whiteToMove][m.start][m.end];
            if(m.start == tablemove.start && m.end == tablemove.end && m.flag == tablemove.flag) priorities[i] = TABLEMOVE_PRIORITY;
        }

        int legals = 0;
        int bestScore = -20000;
        move bestMove;
        bool raisedAlpha = false;

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

            
            // PVS
            int score = 0;
            if (legals == 1) {
                score = -alphabeta(nextBoard, -beta, -alpha, depth-1);
            } else {
                //LMR
                int reduction = ((legals / 16) + (depth / 6) + (legals > 4));
                if ((b->squares[m.end] != EMPTY) || pv || b->inCheck) reduction = 0;
                score = -alphabeta(nextBoard, -alpha-1, -alpha, depth-1-reduction);
                if (score > alpha && reduction > 0) score = -alphabeta(nextBoard, -alpha-1, -alpha, depth-1);
                if (score > alpha && pv) score = -alphabeta(nextBoard, -beta, -alpha, depth-1);
            } 

            delete nextBoard;

            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
                if (score > alpha) {
                    raisedAlpha = true;
                    alpha = score;
                    if (score >= beta) { 
                        if(b->squares[m.end] == EMPTY) history[b->whiteToMove][m.start][m.end] += depth * depth;
                        break;
                    }
                }
            }
        }

        if (legals == 0) {
            pop();
            if (b->inCheck) return -MATE_SCORE + ply;
            return DRAW_SCORE;
        }

        int8_t boundtype = EXACT;
        if (!raisedAlpha) boundtype = UPPERBOUND;
        if (bestScore >= beta) boundtype = LOWERBOUND;

        tableset(b, bestMove, depth, bestScore, boundtype);
        

        pop();
        return bestScore;
    }
};

void iterativeSearch(board* b, int searchTime, std::vector<unsigned long long> prevHashs) {
    searcher s = searcher{};
    s.startTime = std::chrono::steady_clock::now();
    s.timeAlloc = searchTime;
    s.prev = prevHashs;
    
    int depth;
    for(depth=1;depth<30;depth++){
        int score = s.alphabeta(b, -MATE_SCORE, MATE_SCORE, depth);
        std::cout << "info depth " << depth << " cp " << score << " time " << s.ellapsedTime() << " nodes " << s.nodes << " ";
        printpv(b);
        std::cout << std::endl;
        if (s.outOfTime() || score > 9000) break;  
    }

    ttentry* tentry = tableget(b->getHash());
    move chosenMove = tentry->tableMove;
    std::cout << "bestmove ";
    chosenMove.print();
    std::cout << std::endl;
}
