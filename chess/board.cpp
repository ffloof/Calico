#include <iostream>
#include <map>
#include <cstring>
#include <string>
#include <vector>
#include <random>

/*
23444432
34666643
46888864
46888864
46888864
46888864
34666643
23444432
*/

const int color[2] = {-1,1};
const int N = -16, S = 16, W = -1, E = 1;
const int8_t EMPTY = 0, PAWN = 6, KNIGHT = 5, BISHOP = 4, ROOK = 3, QUEEN = 2, KING = 1;
const int8_t BLACK = 6;
const int8_t piececolor[2] = {6,0};
const int A8 = 0, H8 = 7, A1 = 112, H1 = 119;

const int lookup[256] = {
    N+W, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N+E, 0,
    0, N+W, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, N+E, 0, 0, 
    0, 0, N+W, 0, 0, 0, 0, N, 0, 0, 0, 0, N+E, 0, 0, 0, 
    0, 0, 0, N+W, 0, 0, 0, N, 0, 0, 0, N+E, 0, 0, 0, 0, 
    0, 0, 0, 0, N+W, 0, 0, N, 0, 0, N+E, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, N+W, 0, N, 0, N+E, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, N+W, N, N+E, 0, 0, 0, 0, 0, 0, 0, 
    W, W, W, W, W, W, W, 0, E, E, E, E, E, E, E, 0, 
    0, 0, 0, 0, 0, 0, S+W, S, S+E, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, S+W, 0, S, 0, S+E, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, S+W, 0, 0, S, 0, 0, S+E, 0, 0, 0, 0, 0, 
    0, 0, 0, S+W, 0, 0, 0, S, 0, 0, 0, S+E, 0, 0, 0, 0, 
    0, 0, S+W, 0, 0, 0, 0, S, 0, 0, 0, 0, S+E, 0, 0, 0, 
    0, S+W, 0, 0, 0, 0, 0, S, 0, 0, 0, 0, 0, S+E, 0, 0, 
    S+W, 0, 0, 0, 0, 0, 0, S, 0, 0, 0, 0, 0, 0, S+E, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

std::string squareToStr(int n){
    int rank = n >> 4;
    int file = n & 7;    
    return {char('a'+file),char('1'+(7-rank))};
}

int strToSquare(std::string str){
    int file = str[0] - 'a';
    int rank = 7-(str[1] - '1');
    return (rank * 16) + file;
}

const std::string pieceChars = ".KQRBNPkqrbnp";

char pieceToChar(int8_t piece){
    return pieceChars[piece];
}

int8_t charToPiece(char c){
   return pieceChars.find(c);
}

struct move {
    int8_t start;
    int8_t end;

    void print(int8_t promoflag=EMPTY){
        std::cout << squareToStr(start); 
        std::cout << squareToStr(end);
        if (promoflag != EMPTY) std::cout << pieceToChar(promoflag);
    }
};

move NULLMOVE = move{9,9};

struct board {
    int8_t squares[128];
    int kings[2]; 
    int enpassant;
    uint64_t hash;
    bool shortCastle[2];
    bool longCastle[2];
    bool whiteToMove;
    bool inCheck;

    int score;
    int ply;

    std::vector<move> GenerateMoves(bool capturesOnly=false){
        std::vector<move> moves;
        //if (capturesOnly) moves.reserve(16); //TODO: check if it makes sense to preallocate a specific amount and what it should be
        //else moves.reserve(64);

        int advance = whiteToMove ? N : S;

        for (int i=A8; i<=H1; i++) {
            int8_t piece = squares[i];
            if (piece == EMPTY) continue;
            if ((piece <= 6) != whiteToMove) continue;

            int8_t piecetype = piece;
            if (piece > 6) piecetype -= 6;

            switch(piecetype){
                case PAWN: {
                        if (valid(i+advance+W)) addMove(&moves, i, i+advance+W, true);
                        if (valid(i+advance+E)) addMove(&moves, i, i+advance+E, true);
                        if (squares[i+advance] == EMPTY) {
                            addMove(&moves, i, i+advance, capturesOnly);
                            if (squares[i+advance+advance] == EMPTY && isHomeRow(i)) {
                                addMove(&moves, i, i+advance+advance, capturesOnly);
                            }
                        }
                        break;
                    }
                case KNIGHT:
                    PieceMoves(&moves, i, std::vector<int>{N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S}, false, capturesOnly);
                    break;
                case BISHOP:
                    PieceMoves(&moves, i, std::vector<int>{N+W,N+E,S+W,S+E}, true, capturesOnly);
                    break;
                case ROOK: {
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W}, true, capturesOnly);
                    break;
                    }
                case QUEEN:
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, true, capturesOnly);
                    break;
                case KING:
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, false, capturesOnly);
                    break;
            }
        }


             

        

        if (enpassant != 0) {
            for (int8_t origin : {enpassant-advance+W, enpassant-advance+E}) {
                if (squares[origin] == PAWN + piececolor[whiteToMove]) {
                    addMove(&moves, origin, enpassant, capturesOnly);
                }
            }
        }

        int kingIdx = kings[whiteToMove];        
        inCheck = attacked(kingIdx);

        if (!capturesOnly && !inCheck) {
            
            if (shortCastle[whiteToMove] && squares[kingIdx + E] == EMPTY && squares[kingIdx + E + E] == EMPTY) {
                addMove(&moves, kingIdx, kingIdx+E+E, false);
            }
            if (longCastle[whiteToMove] && squares[kingIdx + W] == EMPTY && squares[kingIdx + W + W] == EMPTY && squares[kingIdx + W + W + W] == EMPTY) {
                addMove(&moves, kingIdx, kingIdx+W+W, false);
            }
        }

        return moves;
    }

    bool valid(int n) {
        return (n & 0x88) == 0;
    }

    void PieceMoves(std::vector<move>* moves, int start, std::vector<int> pattern, bool ray, bool capturesOnly) {
        for (int direction: pattern) {
            for (int end=start+direction; valid(end); end+=direction) {
                addMove(moves, start, end, capturesOnly);
                if (squares[end] != EMPTY || !ray) break;
            }
        }
    }

    void addMove(std::vector<move>* moves, int8_t start, int8_t end, bool capturesOnly) {
        if (capturesOnly && squares[end] == EMPTY) return;
        if (squares[end] != EMPTY && (whiteToMove == (squares[end] <= 6))) return;

        moves->push_back(move{start,end});
    }

    bool isHomeRow(int index) {
        int rank = index >> 4;
        if (whiteToMove) return rank == 6;
        return rank == 1;
    }

    bool attacked(int index) {
        std::vector<int> pawnAttacks = { S+W,S+E };
        if (whiteToMove) pawnAttacks = { N+W,N+E };
        
        bool attacked =  scan(PAWN + piececolor[!whiteToMove], index, pawnAttacks, false) ||
            scan(KNIGHT + piececolor[!whiteToMove], index, std::vector<int>{N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S}, false) ||
            scan(BISHOP + piececolor[!whiteToMove], index, std::vector<int>{N+W,N+E,S+W,S+E}, true) ||
            scan(ROOK + piececolor[!whiteToMove], index, std::vector<int>{N,S,E,W}, true) ||
            scan(QUEEN + piececolor[!whiteToMove], index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, true) ||
            scan(KING + piececolor[!whiteToMove], index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, false);
        return attacked;
    }

    bool fastAttacked(int index, int start) {
        int delta = start - index;
        int checkdir = lookup[delta + 0x77];
        if (checkdir == 0) return false;  
        else if (checkdir == N || checkdir == S || checkdir == E || checkdir == W) return scan(ROOK + piececolor[!whiteToMove], index, std::vector<int>{checkdir}, true) || scan(QUEEN + piececolor[!whiteToMove], index, std::vector<int>{checkdir}, true);
        return scan(BISHOP + piececolor[!whiteToMove], index, std::vector<int>{checkdir}, true) || scan(QUEEN + piececolor[!whiteToMove], index, std::vector<int>{checkdir}, true);
    }

    bool scan(int8_t find, int start, std::vector<int> pattern, bool ray){
        for (int direction: pattern) {
            for (int end = start + direction; valid(end); end += direction) {
                int8_t piece = squares[end];
                if (!ray || piece != EMPTY) {
                    if (piece == find) return true;
                    break;
                }
            }
        }
        return false;
    }

    // Defined in table.cpp
    uint64_t getHash();
    void updateHash(int index, int8_t oldPiece, int8_t newPiece);

    // Defined in eval.cpp
    void updateEval(int from, int to, int8_t piece, int changeIndex);

    void edit(int index, int8_t newPiece){
        int8_t oldPiece = squares[index];
        updateHash(index, oldPiece, newPiece);
        squares[index] = newPiece;
    }

    void print(){
        for (int i=0;i<128;i++){
            if (i == enpassant && enpassant != 0) std::cout << "x";
            else std::cout << pieceToChar(squares[i]);
            if (i%16 == 7) {
                i += 8;
                std::cout << std::endl;
            }
            
        }
        std::cout << getHash() << std::endl;
    }
};



board* apply(board* oldBoard, move m){
    board* cBoard = new board;
    if (cBoard == nullptr) {
        std::cout << "nullpointerino" << std::endl;
    }
    std::memcpy(cBoard, oldBoard, sizeof(*oldBoard));

    int8_t movingPiece = cBoard->squares[m.start];
    int8_t victimPiece = cBoard->squares[m.end];

    bool isWhite = cBoard->whiteToMove;
    cBoard->enpassant = 0;
    cBoard->ply++;

    cBoard->updateEval(m.start, m.end, movingPiece, 0);
    if (victimPiece > 0) cBoard->updateEval(m.end, 127, victimPiece, 1);

    if (movingPiece == PAWN || movingPiece == PAWN + BLACK) {
        if (abs(m.end-m.start) == (2 * S)) {
            // DOUBLE PUSH
            cBoard->enpassant = m.end + (S*color[isWhite]);
        }else if (abs(m.end-m.start) != S && cBoard->squares[m.end] == EMPTY) {
            // EN PASSANT
            int exPawnSquare = m.end + (S*color[isWhite]);
            cBoard->edit(exPawnSquare, EMPTY);
            cBoard->updateEval(exPawnSquare, 127, PAWN+piececolor[!isWhite], 1);
        }

        // Promotion if a pawn ends on either of the back ranks, promote, always to a queen
        if (m.end <= H8 || m.end >= A1) {
            cBoard->edit(m.start, QUEEN + piececolor[isWhite]); // Have to make a weird call to edit to make sure hash and psqt are updated correctly
            cBoard->updateEval(m.start, 127, movingPiece, 0);
            cBoard->updateEval(0, m.end, QUEEN+piececolor[isWhite], victimPiece > 0 ? 2 : 1);
            movingPiece = QUEEN + piececolor[isWhite];
        }   
    }

    cBoard->edit(m.end, movingPiece);
    cBoard->edit(m.start, EMPTY);

    bool isKingMoving = ((movingPiece == KING) || (movingPiece == KING + BLACK));

    if (!isKingMoving) {
        /*if (cBoard->attacked(cBoard->kings[isWhite])) {
            delete cBoard;
            return nullptr;
        }
        */
        if (!cBoard->inCheck) {
            if (cBoard->fastAttacked(cBoard->kings[isWhite], m.start)) {
                delete cBoard;
                return nullptr;
            }
        } else if (cBoard->attacked(cBoard->kings[isWhite])) {
            delete cBoard;
            return nullptr;
        }
    } else { 
        if (m.end - m.start == W + W) {
            if (cBoard->attacked(cBoard->kings[isWhite]+W)) {
                delete cBoard;
                return nullptr;
            }
            //CASTLE LONG
            cBoard->edit(m.end + W + W, EMPTY);
            cBoard->edit(m.end + E, ROOK+piececolor[isWhite]);
            cBoard->updateEval(m.end + W + W, m.end + E, ROOK+piececolor[isWhite], 1);
        }
        if (m.end - m.start == E + E) {
            if (cBoard->attacked(cBoard->kings[isWhite]+E)) {
                delete cBoard;
                return nullptr;
            }
            //CASTLE SHORT
            cBoard->edit(m.end + E, EMPTY);
            cBoard->edit(m.end + W, ROOK+piececolor[isWhite]);
            cBoard->updateEval(m.end + W, m.end + E, ROOK+piececolor[isWhite], 1);
        }
        
        cBoard->longCastle[isWhite] = false;
        cBoard->shortCastle[isWhite] = false;
        cBoard->kings[isWhite] = m.end;

        if (cBoard->attacked(cBoard->kings[isWhite])) {
            delete cBoard;
            return nullptr;
        }
    }

    if (m.start == H1 || m.end == H1) cBoard->shortCastle[1] = false;
    if (m.start == H8 || m.end == H8) cBoard->shortCastle[0] = false;
    if (m.start == A1 || m.end == A1) cBoard->longCastle[1] = false;
    if (m.start == A8 || m.end == A8) cBoard->longCastle[0] = false;

    if (m.start == m.end) cBoard->updateEval(127,127,EMPTY,0); // Incase of null move

    cBoard->whiteToMove = !cBoard->whiteToMove;
    
    return cBoard;   
}

std::string trim(std::string str){
    size_t firstNonSpace = str.find_first_not_of(' ');
    size_t lastNonSpace = str.find_last_not_of(' ');
    if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos) {
        return str.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
    }
    return "";
}

std::string beforeWord(std::string str, std::string word) {
    size_t found = str.find(word);
    if (found != std::string::npos) {
        return trim(str.substr(0, found));
    }
    return trim(str.substr(0, str.length()));
}

std::string afterWord(std::string str, std::string word) {
    size_t found = str.find(word);
    if (found != std::string::npos) {
        return trim(str.substr(found + word.length()));
    }
    return "";
}

// TODO: make this a proper constructor
board newBoard(std::string fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
    const std::map<char, int8_t> charToOffset = {{'/', 8},{'1', 1},{'2', 2},{'3', 3},{'4', 4},{'5', 5},{'6', 6},{'7', 7},{'8', 8},};
    board b = board{};    

    std::string position = beforeWord(fen, " ");

    int8_t i = 0;
    for (char pchar : position) {
        if (charToOffset.count(pchar)) {
            i += charToOffset.at(pchar);
        } else {
            b.edit(i,charToPiece(pchar));
            if (pchar == 'k') b.kings[0] = i;
            if (pchar == 'K') b.kings[1] = i;
            i += 1;
        }
    }

    std::string details = afterWord(fen, " ");

    b.whiteToMove = details[0] == 'w';

    std::string rights = beforeWord(afterWord(details, " "), " ");

    b.shortCastle[1] = rights.find("K") != std::string::npos;
    b.shortCastle[0] = rights.find("k") != std::string::npos;
    b.longCastle[1] = rights.find("Q") != std::string::npos;
    b.longCastle[0] = rights.find("q") != std::string::npos;
    
    std::string epstr = beforeWord(afterWord(afterWord(details, " "), " "), " ");
    if (epstr != "-" && epstr != "" && epstr != " ") b.enpassant = strToSquare(epstr);

    // Should I add halfmove clock?
    return b;
}

int perft(board* b, int depth){
    if (depth == 0) return 1;

    int nodes = 0;

    std::vector<move> moves = b->GenerateMoves();
    for (move m : moves) {
        board* nextBoard = apply(b, m);
        if (nextBoard != nullptr) nodes += perft(nextBoard, depth-1);
        delete nextBoard;
    }

    return nodes;
}

board* applyMoveStr(board* b, std::string moveStr){
    if(moveStr.length() < 4) return nullptr;
    int8_t start = strToSquare(moveStr.substr(0,2));
    int8_t end = strToSquare(moveStr.substr(2,4));
    return apply(b, move{start,end});
}
