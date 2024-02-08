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
const int8_t EMPTY = 0, PAWN = 2, KNIGHT = 4, BISHOP = 6, ROOK = 8, QUEEN = 10, KING = 12;
const int A8 = 0, H8 = 7, A1 = 112, H1 = 119; 

bool valid(int index) {
    return (index & 0x88) == 0;
}

std::string squareToStr(int n){
    int rank = n >> 4;
    int file = n & 7;    
    return {char('a'+file),char('1'+(7-rank))};
}

int strToSquare(std::string str){
    int file = str[0] - 'a';
    int rank = 7-(str[1] - '1');
    return (rank << 4) | file;
}

const std::string pieceChars = "..pPnNbBrRqQkK";

char pieceToChar(int8_t piece){
    return pieceChars[piece];
}

int8_t charToPiece(char c){
   return pieceChars.find(c);
}

struct move {
    int8_t start;
    int8_t end;
    int8_t flag;

    void print(){
        std::cout << squareToStr(start); 
        std::cout << squareToStr(end);
        if (flag != EMPTY) std::cout << pieceToChar(-flag);
    }
};

move NULLMOVE = move{9,9,EMPTY};

struct board {
    int8_t squares[128];
    int kings[2]; 
    int enpassant;
    uint64_t hash;
    bool shortCastle[2];
    bool longCastle[2];
    bool whiteToMove;
    bool inCheck;
    int mobilities[2];

    int earlyScore;
    int lateScore;
    int phase;

    std::vector<move> GeneratesMoves(bool capturesOnly=false){
        std::vector<move> moves = {};

        int advance = N * color[whiteToMove];
        int sideMobility = 0;

        for (int i=0; i<128; i++) {
            if (squares[i] == EMPTY) continue;
            if ((squares[i] & 1) != whiteToMove) continue;

            int8_t piecetype = squares[i] & 14;

            switch(piecetype){
                
                case PAWN:
                    addPawnMove(&moves, i, i+advance+W, true);
                    addPawnMove(&moves, i, i+advance+E, true);
                    if (squares[i+advance] == EMPTY) {
                        addPawnMove(&moves, i, i+advance, capturesOnly);
                        if (squares[i+advance+advance] == EMPTY && isHomeRow(i)) {
                            addMove(&moves, i, i+advance+advance, capturesOnly);
                        }
                    }
                    break;
                case KNIGHT:
                    PieceMoves(&moves, i, std::vector<int>{N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S}, false, capturesOnly);
                    break;
                case BISHOP:
                    sideMobility += 2 * PieceMoves(&moves, i, std::vector<int>{N+W,N+E,S+W,S+E}, true, capturesOnly);
                    break;
                case ROOK:
                    sideMobility += 3 * PieceMoves(&moves, i, std::vector<int>{N,S,E,W}, true, capturesOnly);
                    break;
                case QUEEN:
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, true, capturesOnly);
                    break;
                case KING:
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, false, capturesOnly);
                    break;
            }
        }        

        if (enpassant != 0) {
            for (int8_t origin : std::vector<int>{enpassant-advance+W, enpassant-advance+E}) {
                if (squares[origin] == PAWN + whiteToMove) {
                    addMove(&moves, origin, enpassant, capturesOnly);
                }
            }
        }

        int kingIdx = kings[whiteToMove];
        bool canShortCastle = shortCastle[whiteToMove];
        bool canLongCastle = longCastle[whiteToMove];
        inCheck = attacked(kingIdx);

        if (!capturesOnly && !inCheck) {
            if (canShortCastle && squares[kingIdx + E] == EMPTY && squares[kingIdx + E + E] == EMPTY && !attacked(kingIdx + E)) {
                addMove(&moves, kingIdx, kingIdx+E+E, false);
            }
            if (canLongCastle && squares[kingIdx + W] == EMPTY && squares[kingIdx + W + W] == EMPTY && squares[kingIdx + W + W + W] == EMPTY && !attacked(kingIdx + W)) {
                addMove(&moves, kingIdx, kingIdx+W+W, false);
            }
        }

        mobilities[whiteToMove] = sideMobility;
        return moves;
    }

    int PieceMoves(std::vector<move>* moves, int start, std::vector<int> pattern, bool ray, bool capturesOnly) {
        int mobility = 0;
        for (int direction: pattern) {
            for (int end=start+direction; valid(end); end+=direction) {
                addMove(moves, start, end, capturesOnly);
                mobility++;
                if (squares[end] != EMPTY || !ray) break;
            }
        }
        return mobility;
    }

    void addPawnMove(std::vector<move>* moves, int start, int end, bool capturesOnly) {
        if (isPromotionRow(end)) {
            for (int8_t promotion : std::vector<int8_t>{QUEEN, ROOK, KNIGHT, BISHOP}) 
                addMove(moves, start, end, capturesOnly, promotion);
            return;
        }
        addMove(moves, start, end, capturesOnly);
    }

    void addMove(std::vector<move>* moves, int8_t start, int8_t end, bool capturesOnly, int8_t flag=EMPTY) {
        
        if (!valid(end)) return;
        if (capturesOnly && squares[end] == EMPTY) return;
        if (squares[end] != EMPTY && (whiteToMove == (squares[end] & 1))) return;

        moves->push_back(move{start,end,flag});
    }

    bool isHomeRow(int index) {
        int rank = index >> 4;
        if (whiteToMove) return rank == 6;
        return rank == 1;
    }

    bool isPromotionRow(int index) {
        int rank = index >> 4;
        if (whiteToMove) return rank == 0;
        return rank == 7;
    }

    bool attacked(int index) {
        std::vector<int> pawnAttacks = { S+W,S+E };
        if (whiteToMove) pawnAttacks = { N+W,N+E };
        
        bool attacked =  scan(PAWN + !whiteToMove, index, pawnAttacks, false) ||
            scan(KNIGHT + !whiteToMove, index, std::vector<int>{N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S}, false) ||
            scan(BISHOP + !whiteToMove, index, std::vector<int>{N+W,N+E,S+W,S+E}, true) ||
            scan(ROOK + !whiteToMove, index, std::vector<int>{N,S,E,W}, true) ||
            scan(QUEEN + !whiteToMove, index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, true) ||
            scan(KING + !whiteToMove, index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, false);
        return attacked;
    }

    bool scan(int8_t find, int start, std::vector<int> pattern, bool ray){
        for (int direction: pattern) {
            for (int end = start + direction; valid(end); end += direction) {
                int8_t piece = squares[end];
                if (piece == find) return true;
                if (!ray || piece != EMPTY) break;
            }
        }
        return false;
    }

    // Defined in table.cpp
    uint64_t getHash();
    void updateHash(int index, int8_t oldPiece, int8_t newPiece);

    // Defined in eval.cpp
    void updateEval(int index, int8_t oldPiece, int8_t newPiece);

    void edit(int index, int8_t piece){
        int8_t oldPiece = squares[index];
        updateHash(index, oldPiece, piece);
        updateEval(index, oldPiece, piece);
        squares[index] = piece;
    }

    void print(){
        for (int i=0;i<128;i++){
            if (i%16 == 15) std::cout << std::endl;
            if (i == enpassant && enpassant != 0) std::cout << "-";
            else if (i%16 < 8) std::cout << pieceToChar(squares[i]);
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

    bool isWhite = cBoard->whiteToMove;
    cBoard->enpassant = 0;
    if ((movingPiece & 14) == PAWN) {
        if (abs(m.end-m.start) == (2 * S)) {
            // DOUBLE PUSH
            cBoard->enpassant = m.end + (S*color[isWhite]);
        }else if (abs(m.end-m.start) != S && cBoard->squares[m.end] == EMPTY) {
            // EN PASSANT
            cBoard->edit(m.end + (S*color[isWhite]), EMPTY);
        }
    }

    cBoard->edit(m.end, movingPiece);
    cBoard->edit(m.start, EMPTY);

    if ((movingPiece & 14) == KING) {
        cBoard->kings[isWhite] = m.end;
        cBoard->longCastle[isWhite] = false;
        cBoard->shortCastle[isWhite] = false;
        if (m.end - m.start == W + W) {
            //CASTLE LONG
            cBoard->edit(m.end + W + W, EMPTY);
            cBoard->edit(m.end + E, ROOK+isWhite);
        }
        if (m.end - m.start == E + E) {
            //CASTLE SHORT
            cBoard->edit(m.end + E, EMPTY);
            cBoard->edit(m.end + W, ROOK+isWhite);
        }
    }

    

    // TODO: rename flag to something more descriptive like promoflag
    if (m.flag != EMPTY) {
        cBoard->edit(m.end, m.flag + isWhite);
    }

    if (cBoard->attacked(cBoard->kings[isWhite])) {
        delete cBoard;
        return nullptr;
    }

    if (m.start == H1 || m.end == H1) cBoard->shortCastle[1] = false;
    if (m.start == H8 || m.end == H8) cBoard->shortCastle[0] = false;
    if (m.start == A1 || m.end == A1) cBoard->longCastle[1] = false;
    if (m.start == A8 || m.end == A8) cBoard->longCastle[0] = false;

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

    std::vector<move> moves = b->GeneratesMoves();
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
    int8_t flag = EMPTY;
    if(moveStr.length() == 5) flag = abs(charToPiece(moveStr[4]));
    return apply(b, move{start,end,flag});
}
