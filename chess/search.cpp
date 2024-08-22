#include <vector>
#include <chrono>

const int MATE_SCORE = 10000;
const int DRAW_SCORE = 0;

const int TABLEMOVE_PRIORITY = 1000000000;
const int CAPTURE_PRIORITY   = 100000000;
const int KILLER_PRIORITY    = 100;
const int FOLLOW_PRIORITY    = 10;
const int COUNTER_PRIORITY   = 10;

const int deltas[7] = {0, 0, 1300, 700, 450, 400, 150 };
int64_t history[13][128];

struct searcher {
    int nodes;
    int qnodes;
    std::vector<uint64_t> prevHashs;
    move moveStack[255];
    uint64_t repetition[255];
    move killers[64];
    move followTable[128][128];
    move counterTable[128][128];
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    int timeAlloc;


    // TODO: add evals here

    void push(uint64_t hash, int ply){
        repetition[ply] = hash;
    }

    bool isRepetition(uint64_t hash, int ply){
        if (ply == 0) return false;

        for(int i=0;i<ply;i++){
            if (repetition[i] == hash) return true;
        }

        for (int i=1;i<prevHashs.size();i++) {
            if (prevHashs[i] == hash) return true; 
        }

        return false;
    }

    int ellapsedTime(){
        auto currentTime = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    }

    bool outOfTime(bool inSearch=true){
        if (inSearch && nodes % 1024 != 0) return false; // Calls to time are expensive avoid doing them too often
        if (!inSearch) return ellapsedTime() > timeAlloc / 10; // Soft limit
        return ellapsedTime() > timeAlloc; // Hard limit
    }

    int qsearch(board* b, int alpha, int beta) {
        qnodes++;

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

        if (bestScore >= beta) return bestScore;
        if (standpat > alpha) alpha = standpat;

        
        std::vector<move> moves = b->GenerateMoves(true);
        std::vector<int> priorities(moves.size());

        for (int i=0;i<moves.size();i++){
            move m = moves[i];
            priorities[i] = ((16-b->squares[m.end]) * 128) - (16-b->squares[m.start]) + 16;
        }



        move bestMove = NULLMOVE;
        bool raisedAlpha = true;
        int legals = 0;
        for (int i=0;i<moves.size();i++){
            //if (outOfTime()) throw 0;
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

            int8_t piecetype = b->squares[m.end];
            if (piecetype > 6) piecetype -= 6;
            // Delta pruning
            // if (standpat + deltas[piecetype] <= alpha) break;

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
        tableset(hash, bestMove, 0, bestScore, boundtype, false);
    
        return bestScore;
    }

    int alphabeta(board* b, int alpha, int beta, int depth){        
        if (depth <= 0) {
            int deltaq = qnodes;
            int qscore = qsearch(b, alpha, beta);
            deltaq -= qnodes;
            deltaq = -deltaq;
            if (deltaq > 200) {
                std::cout << "SCREAM " << deltaq << std::endl;
                b->print();
            } 
            return qscore;
        }

        nodes++;

        

        bool pv = beta > alpha + 1;

        uint64_t hash = b->getHash();
        if (isRepetition(hash, b->ply)) return DRAW_SCORE;
        push(hash, b->ply);

        ttentry* tentry = tableget(hash);

        if (tentry == nullptr && depth > 3) {
            if(!pv) depth -= 1 - (depth/ 8); // Internal iterative reduction
        }


        move tablemove;
        if (tentry != nullptr) {
            tablemove = tentry->tableMove;
            if (depth <= tentry->depth){ 
                if ((tentry->bound == EXACT)
                || (tentry->bound == LOWERBOUND && tentry->score >= beta) 
                || (tentry->bound == UPPERBOUND && tentry->score <= alpha)) { 
                    return tentry->score;
                }
            }
        }

        std::vector<move> moves = b->GenerateMoves();
        int eval = evaluate(b);

        if (eval > beta && !pv && !b->inCheck) {
            // Reverse futility pruning (RFP)
            if (depth <= 4 && (eval - (60 * depth) >= beta)) return eval - (60 * depth);
            // Null move pruning (NMP)
            if (depth > 4) {
                board* nmBoard = apply(b, NULLMOVE);
                if (nmBoard != nullptr) {
                    int nmScore = -alphabeta(nmBoard, -beta, -alpha, (((depth * 100) + (beta - eval)) / 186) - 1);
                    delete nmBoard;
                    if (nmScore >= beta) return nmScore;
                }
            }
        }

        std::vector<int> priorities(moves.size());
        move killermove = killers[b->ply];
        move followmove;
        move countermove;
        if (b->ply > 0) {
            move ma = moveStack[b->ply - 1];
            countermove = counterTable[ma.start][ma.end];
        }
        if (b->ply > 1) {
            move mb = moveStack[b->ply - 2];
            followmove = followTable[mb.start][mb.end];
        }

        for (int i=0;i<moves.size();i++){
            move m = moves[i];
            priorities[i] = ((16-b->squares[m.end]) * 128) - (16-b->squares[m.start]) + CAPTURE_PRIORITY;
            priorities[i] += history[b->squares[m.start]][m.end];
            if(m.start == tablemove.start && m.end == tablemove.end) priorities[i] = TABLEMOVE_PRIORITY;
            if(m.start == killermove.start && m.end == killermove.end) priorities[i] += KILLER_PRIORITY;
            if(m.start == countermove.start && m.end == countermove.end) priorities[i] += COUNTER_PRIORITY;
            if(m.start == followmove.start && m.end == followmove.end) priorities[i] += FOLLOW_PRIORITY;
        }

        

        int legals = 0;
        int quietsLeft = (depth * depth) + 3;
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

            moveStack[b->ply] = m;

            board* nextBoard = apply(b,m);
            if (nextBoard == nullptr) continue;
            
            legals++;
            
            //if (pv && b->ply == 0) {std::cout << legals << "/" << moves.size() << " ";m.print();std::cout<<std::endl;}

            int nextDepth = b->inCheck ? depth : depth - 1;

            // PVS
            int score = 0;
            if (legals == 1) {
                score = -alphabeta(nextBoard, -beta, -alpha, nextDepth);
            } else {
                //LMR
                //TODO: should we disallow history heuristic to make extensions with fmax
                int reduction = (((legals * 93) + (depth * 144)) / 1000) + (history[b->squares[m.start]][m.end]/172);
                
                if ((b->squares[m.end] != EMPTY) || b->inCheck || reduction < 0) {
                    reduction = 0;
                }

                //if (pv && b->ply == 0) std::cout << depth << " - " << reduction << std::endl;

                score = -alphabeta(nextBoard, -alpha-1, -alpha, nextDepth-reduction);
                if (score > alpha && reduction > 0) score = -alphabeta(nextBoard, -alpha-1, -alpha, nextDepth);
                if (score > alpha && pv) score = -alphabeta(nextBoard, -beta, -alpha, nextDepth);
            } 

            delete nextBoard;

            //if (pv && b->ply == 0) {std::cout << score << " comp " << bestScore << std::endl;}

            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
                if (score > alpha) {
                    raisedAlpha = true;
                    alpha = score;
                    if (score >= beta) { 
                        
                        if(b->squares[m.end] == EMPTY) {
                            // Update killers, countermove and followupmove
                            killers[b->ply] = m;
                            if (b->ply > 0) {
                                move ma = moveStack[b->ply - 1];
                                counterTable[ma.start][ma.end] = m;
                            }
                            if (b->ply > 1) {
                                move mb = moveStack[b->ply - 2];
                                followTable[mb.start][mb.end] = m;
                            }
                            
                            // Update history
                            int updateSize = eval < alpha ? -(depth+1) : depth;
                        
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

            //if (pv && b->ply == 0) {std::cout << "best "; bestMove.print(); std::cout << " comp " << bestScore << std::endl;}

            if (!pv && b->squares[m.end] != 0){
                quietsLeft--;
                if (quietsLeft <= 0) break;
                if (depth <= 4 && eval + 127 * depth < alpha) break;
            }   
        }

        //if (pv && b->ply == 0) { std::cout << "finalmove"; bestMove.print(); std::cout << std::endl;} 

        if (legals == 0) {
            if (b->inCheck) return -MATE_SCORE + b->ply;
            return DRAW_SCORE;
        }

        int8_t boundtype = EXACT;
        if (!raisedAlpha) boundtype = UPPERBOUND;
        if (bestScore >= beta) boundtype = LOWERBOUND;

        tableset(hash, bestMove, depth, bestScore, boundtype, pv);
        
        
        return bestScore;
    }
};

void iterativeSearch(board* b, int searchTime, std::vector<uint64_t> prev) {
    searcher s = searcher{};
    s.startTime = std::chrono::steady_clock::now();
    s.timeAlloc = searchTime;
    s.prevHashs = prev;
    
    ttentry* tentry = tableget(b->getHash());
    int lastscore = 0;
    move chosenMove;
    if (tentry != nullptr) lastscore = tentry->score;
    
    for(int depth=1;depth<100;depth++){
        try {
            int score = s.alphabeta(b, lastscore - 20, lastscore + 20, depth);
            if ((score <= (lastscore - 20)) || ((lastscore + 20) <= score)) score = s.alphabeta(b, -MATE_SCORE, MATE_SCORE, depth);

            std::cout << "info depth " << depth << " score cp " << score << " time " << s.ellapsedTime() + 1 << " nodes " << s.nodes  + s.qnodes  << " ";
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
