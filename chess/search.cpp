#include <vector>
#include <chrono>

const int MATE_SCORE = 10000;
const int DRAW_SCORE = 0;

const int TABLEMOVE_PRIORITY = 1000000000;
const int CAPTURE_PRIORITY   = 100000000;
const int KILLER_PRIORITY    = 10000000;

struct searcher {
    int nodes;
    std::vector<uint64_t> prev;
    uint64_t repetition[255];
    int64_t history[14][120];
    move killers[64];
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    int timeAlloc;


    // TODO: add evals here

    void push(uint64_t hash, int ply){
        repetition[ply] = hash;
        nodes++;
    }

    bool isRepetition(uint64_t hash, int ply){
        if (ply == 0) return false;

        for(int i=0;i<ply;i++){
            if (repetition[i] == hash) return true;
        }

        for (int i=1;i<prev.size();i++) {
            if (prev[i] == hash) return true; 
        }

        return false;
    }

    int ellapsedTime(){
        auto currentTime = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    }

    bool outOfTime(bool inSearch=true){
        if (inSearch && nodes % 1024 != 0) return false; // Calls to time are expensive avoid doing them too often
        if (!inSearch) return ellapsedTime() > timeAlloc / 2; // Soft limit
        return ellapsedTime() > timeAlloc; // Hard limit
    }

    int qsearch(board* b, int alpha, int beta) {
        uint64_t hash = b->getHash();
        push(hash, b->ply);

        ttentry* tentry = tableget(hash);
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
        int bestScore = standpat;
        if (bestScore >= beta) {
            return bestScore;
        }
        
        std::vector<move> moves = b->GenerateMoves(true);
        std::vector<int> priorities(moves.size());

        for (int i=0;i<moves.size();i++){
            move m = moves[i];
            priorities[i] = (b->squares[m.end] * 16) - b->squares[m.start] + 16;
        }

        move bestMove;
        bool raisedAlpha = true;
        int legals = 0;
        for (int i=0;i<moves.size();i++){
            if (outOfTime()) throw 0;
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

            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
            }

            if (score > alpha) {
                raisedAlpha = true;
                alpha = score;
                if (score >= beta) break;
            }
        }

        int8_t boundtype = EXACT;
        if (!raisedAlpha) boundtype = UPPERBOUND;
        if (bestScore >= beta) boundtype = LOWERBOUND;
        tablesetempty(hash, bestMove, bestScore, boundtype);
        
        return bestScore;
    }

    int alphabeta(board* b, int alpha, int beta, int depth){
        if (depth <= 0) {
            return qsearch(b, alpha, beta);
        }

        bool pv = beta > alpha + 1;

        uint64_t hash = b->getHash();
        if (isRepetition(hash, b->ply)) return DRAW_SCORE;
        push(hash, b->ply);

        ttentry* tentry = tableget(hash);

        if (tentry == nullptr && depth > 3) {
            if(!pv) depth--; // Internal iterative reduction

            // Internal iterative deepening
            alphabeta(b, alpha, beta, depth/2);
            tentry = tableget(hash);
        }

        move tablemove;
        if (tentry != nullptr) {
            tablemove = tentry->tableMove;
            if (depth <= tentry->depth && b->ply != 0){ 
                if ((tentry->bound == EXACT)
                || (tentry->bound == LOWERBOUND && tentry->score >= beta) 
                || (tentry->bound == UPPERBOUND && tentry->score <= alpha)) { 
                    return tentry->score;
                }
            }
        }
        
        // TODO: we could call the part that defines inCheck here seperately but then we lose out on mobility evaluation
        std::vector<move> moves = b->GenerateMoves();
        int eval = evaluate(b);
        
        // Reverse futility pruning
        if (!pv && !b->inCheck && depth <= 5 && eval >= beta + (100*depth)) {
            return eval;
        }

        // Null move pruning
        if (!pv && !b->inCheck && depth >= 2 && eval >= beta && b->phase > 8) {
            board* nmBoard = apply(b, NULLMOVE);
            if (nmBoard != nullptr) {
                int nmReduction = 3 + (depth/3);
                int nmScore = -alphabeta(nmBoard, -beta, -alpha, depth - nmReduction);
                delete nmBoard;
                if (nmScore >= beta) {
                    return nmScore;
                }
            }
        }

        std::vector<int> priorities(moves.size());
        move killermove = killers[b->ply];
        for (int i=0;i<moves.size();i++){
            move m = moves[i];
            priorities[i] = (b->squares[m.end] * 16) - b->squares[m.start] + CAPTURE_PRIORITY;
            priorities[i] += history[b->squares[m.start]][m.end];
            if(m.start == tablemove.start && m.end == tablemove.end) priorities[i] = TABLEMOVE_PRIORITY;
            if(m.start == killermove.start && m.end == killermove.end) priorities[i] += KILLER_PRIORITY;
        }

        int legals = 0;
        int quietsLeft = (depth * depth) - depth + 6;
        if (pv) quietsLeft = 100;
        int bestScore = -20000;
        move bestMove;
        bool raisedAlpha = false;

        for (int i=0;i<moves.size();i++){
            if (outOfTime()) throw 1;

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
                //TODO: should we disallow history heuristic to make extensions with fmax
                int reduction = ((legals / 16) + (depth / 6) + (legals > 4)) + (history[b->squares[m.start]][m.end]/200);
                if ((b->squares[m.end] != EMPTY) || pv || b->inCheck || reduction < 0) {
                    reduction = 0;
                } else {
                    quietsLeft -= 1;
                }
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
                        // Update killers
                        killers[b->ply] = m;
                        
                        // Update history
                        int updateSize = depth * depth;
                        if(b->squares[m.end] == EMPTY) {
                            history[b->squares[m.start]][m.end] += updateSize - (updateSize * history[b->squares[m.start]][m.end] / 512);
                            for (int n=0;n<i;n++){
                                move malusMove = moves[n];
                                if (b->squares[malusMove.end] == EMPTY) history[b->squares[malusMove.start]][malusMove.end] -= updateSize + (updateSize * history[b->squares[malusMove.start]][malusMove.end] / 512);
                            }
                        }

                        break;
                    }
                }
            }

            if (quietsLeft <= 0) {
                break;
            }
        }

        if (legals == 0) {
            if (b->inCheck) return -MATE_SCORE + b->ply;
            return DRAW_SCORE;
        }

        int8_t boundtype = EXACT;
        if (!raisedAlpha) boundtype = UPPERBOUND;
        if (bestScore >= beta) boundtype = LOWERBOUND;

        tableset(hash, bestMove, depth, bestScore, boundtype);
        

        return bestScore;
    }
};

void iterativeSearch(board* b, int searchTime, std::vector<uint64_t> prevHashs) {
    searcher s = searcher{};
    s.startTime = std::chrono::steady_clock::now();
    s.timeAlloc = searchTime;
    s.prev = prevHashs;
    
    ttentry* tentry = tableget(b->getHash());
    int lastscore = 0;
    move chosenMove;
    if (tentry != nullptr) lastscore = tentry->score;
    
    for(int depth=1;depth<30;depth++){
        try {
            int score = s.alphabeta(b, lastscore - 25, lastscore + 25, depth);
            if ((score <= (lastscore - 25)) || ((lastscore + 25) <= score)) score = s.alphabeta(b, -MATE_SCORE, MATE_SCORE, depth);

            std::cout << "info depth " << depth << " score cp " << score << " time " << s.ellapsedTime() << " nodes " << s.nodes << " ";
            printpv(b);
            std::cout << std::endl;

            tentry = tableget(b->getHash());
            chosenMove = tentry->tableMove;

            if (s.outOfTime(false) || score > 9000) break;  
        } catch(int err){
            break;
        }
    }    
    std::cout << "bestmove ";
    chosenMove.print();
    std::cout << std::endl;
}
