#include <iostream>
#include <map>
#include <cstring>
#include <string>
#include <vector>
#include <random>

unsigned long long zobrist[13][128];
unsigned long long zobristEP[128];
unsigned long long zobristSTM[2];
unsigned long long zobristCastleS[2][2];
unsigned long long zobristCastleL[2][2];

void initZobrists(){
    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution<uint64_t> rng;

    for(int x=0;x<13;x++){
        for(int y=0;y<128;y++) zobrist[x][y] = rng(mt);
    }

    for(int y=0;y<128;y++) zobristEP[y] = rng(mt);
    for(int y=0;y<2;y++) zobristSTM[y] = rng(mt);

    for(int x=0;x<2;x++) {
        for(int y=0;y<2;y++) zobristCastleS[y][x] = rng(mt);
        for(int y=0;y<2;y++) zobristCastleL[y][x] = rng(mt);
    }
}

const int color[2] = {-1,1};
const int N = -16, S = 16, W = -1, E = 1;
const int8_t EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
const int A8 = 0, H8 = 7, A1 = 112, H1 = 119; 

bool valid(int index) {
    return (index & 0x88) == 0;
}

std::string squareToStr(int n){
    if (!valid(n)) return "--";
    int rank = n >> 4;
    int file = n & 7;    
    return {char('a'+file),char('1'+(7-rank))};
}

int strToSquare(std::string str){
    int file = str[0] - 'a';
    int rank = 7-(str[1] - '1');
    return (rank << 4) | file;
}

char pieceToChar(int8_t piece){
    const std::string pieceChars = "kqrbnp.PNBRQK";
    return pieceChars[piece + 6];
}

int8_t charToPiece(char c){
    const std::map<char, int8_t> convert = {
        {'P', PAWN}, {'N', KNIGHT}, {'B', BISHOP}, {'R', ROOK}, {'Q', QUEEN}, {'K',KING},
        {'p',-PAWN}, {'n',-KNIGHT}, {'b',-BISHOP}, {'r',-ROOK}, {'q',-QUEEN}, {'k',-KING}
    };

    if (convert.count(c)) return convert.at(c);
    return EMPTY;
}

struct move {
    int8_t start;
    int8_t end;
    int8_t flag;

    void print(){
        std::cout << squareToStr(start); 
        std::cout << squareToStr(end);
        if (flag != EMPTY) std::cout << pieceToChar(-flag);
        std::cout << std::endl;
    }
};

struct board {
    int8_t squares[128];
    int kings[2]; 
    int enpassant;
    unsigned long long hash;
    bool shortCastle[2];
    bool longCastle[2];
    bool whiteToMove;
    bool inCheck;

    std::vector<move> GeneratesMoves(bool capturesOnly=false){
        std::vector<move> moves = {};

        int advance = N * color[whiteToMove];

        for (int i=0; i<128; i++) {
            if (squares[i] == EMPTY) continue;
            if (squares[i] * color[whiteToMove] < 0) continue;
            
            int8_t piecetype = abs(squares[i]);
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
                    PieceMoves(&moves, i, std::vector<int>{N+W,N+E,S+W,S+E}, true, capturesOnly);
                    break;
                case ROOK:
                    PieceMoves(&moves, i, std::vector<int>{N,S,E,W}, true, capturesOnly);
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
                if (squares[origin] == PAWN * color[whiteToMove]) {
                    addMove(&moves, origin, enpassant, capturesOnly);
                }
            }
        }

        int kingIdx = kings[whiteToMove];
        bool canShortCastle = shortCastle[whiteToMove];
        bool canLongCastle = longCastle[whiteToMove];
        bool inCheck = attacked(kingIdx);

        if (!capturesOnly && !inCheck) {
            if (canShortCastle && squares[kingIdx + E] == EMPTY && squares[kingIdx + E + E] == EMPTY && !attacked(kingIdx + E) && !attacked(kingIdx + E + E)) {
                addMove(&moves, kingIdx, kingIdx+E+E, false);
            }
            if (canLongCastle && squares[kingIdx + W] == EMPTY && squares[kingIdx + W + W] == EMPTY && squares[kingIdx + W + W + W] == EMPTY && !attacked(kingIdx + W) && !attacked(kingIdx + W + W)) {
                addMove(&moves, kingIdx, kingIdx+W+W, false);
            }
        }

        return moves;
    }

    void PieceMoves(std::vector<move>* moves, int start, std::vector<int> pattern, bool ray, bool capturesOnly){
        for (int direction: pattern) {
            for (int end=start+direction; valid(end); end+=direction) {
                addMove(moves, start, end, capturesOnly);
                if (squares[end] != EMPTY || !ray) break;
            }
        }
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
        if (squares[start] * squares[end] > 0) return;
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
        int attackerColor = color[!whiteToMove];
        std::vector<int> pawnAttacks = { S+W, S+E };
        if (whiteToMove) pawnAttacks = { N+W,N+E };
        
        return scan(PAWN * attackerColor, index, pawnAttacks, false) ||
            scan(KNIGHT * attackerColor, index, std::vector<int>{N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S}, false) ||
            scan(BISHOP * attackerColor, index, std::vector<int>{N+W,N+E,S+W,S+E}, true) ||
            scan(ROOK * attackerColor, index, std::vector<int>{N,S,E,W}, true) ||
            scan(QUEEN * attackerColor, index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, true) ||
            scan(KING * attackerColor, index, std::vector<int>{N,S,E,W,N+W,N+E,S+W,S+E}, false);
    }

    bool scan(int8_t find, int start, std::vector<int> pattern, bool ray){
        for (int direction: pattern) {
            for (int end = start + direction; valid(end); end += direction) {
                int8_t piece = squares[end];
                if (piece == find) return true;
                if (!ray || piece != 0) break;
            }
        }
        return false;
    }

    unsigned long long getHash(){
        // We add to the hash the sidetomove, enpassant, and castling rights
        // You could also try to keep these updated incrementally but I am too lazy
        return (hash ^ zobristEP[enpassant] ^ zobristSTM[whiteToMove] ^ zobristCastleS[0][shortCastle[0]] ^ zobristCastleS[1][shortCastle[1]] ^ zobristCastleL[0][longCastle[0]] ^ zobristCastleL[1][longCastle[1]]);
    }

    void edit(int index, int8_t piece){
        int8_t oldPiece = squares[index];
        if (oldPiece != EMPTY) hash ^= zobrist[oldPiece+6][index];
        if (piece != EMPTY) hash ^= zobrist[piece+6][index];
        
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
    if (abs(movingPiece) == PAWN) {
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

    if (abs(movingPiece) == KING) {
        cBoard->kings[isWhite] = m.end;
        cBoard->longCastle[isWhite] = false;
        cBoard->shortCastle[isWhite] = false;
        if (m.end - m.start == W + W) {
            //CASTLE LONG
            cBoard->edit(m.end + W + W, EMPTY);
            cBoard->edit(m.end + E, ROOK*color[isWhite]);
        }
        if (m.end - m.start == E + E) {
            //CASTLE SHORT
            cBoard->edit(m.end + E, EMPTY);
            cBoard->edit(m.end + W, ROOK*color[isWhite]);
        }
    }

    // TODO: rename flag to something more descriptive like promoflag
    if (m.flag != EMPTY) {
        cBoard->edit(m.end, m.flag * color[isWhite]);
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

    // TODO: enpassant! halfmove clock?

    std::string epstr = beforeWord(afterWord(afterWord(details, " "), " "), " ");

    if (epstr != "-" && epstr != "" && epstr != " ") b.enpassant = strToSquare(epstr);

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

    int8_t piece = abs(b->squares[start]);

    /*
    if(piece == KING) {
        if (end - start == W + W) flag = CASTLELONG;
        if (end - start == E + E) flag = CASTLESHORT;
    }

    if(piece == PAWN){
        if (abs(end-start) == (2 * S)) flag = DOUBLEPUSH;
        else if (abs(end-start) != S && b->squares[end] == EMPTY) flag = ENPASSANT;
    }*/
    return apply(b, move{start,end,flag});
}