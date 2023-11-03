#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <chrono>

const int color[2] = {-1,1};
const int N = -16, S = 16, W = -1, E = 1;
const int8_t EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
const int8_t BASIC = 0, CASTLELONG = 1, CASTLESHORT = 2, DOUBLEPUSH = 3, ENPASSANT = 4, NULLMOVE = 5, PROMOTEQUEEN = 6, PROMOTEROOK = 7, PROMOTEBISHOP = 8, PROMOTEKNIGHT = 9;
const int A8 = 0, H8 = 7, A1 = 112, H1 = 119; 

const std::string squareName[128] = {
    "a8","b8","c8","d8","e8","f8","g8","h8","--","--","--","--","--","--","--","--",
    "a7","b7","c7","d7","e7","f7","g7","h7","--","--","--","--","--","--","--","--",
    "a6","b6","c6","d6","e6","f6","g6","h6","--","--","--","--","--","--","--","--",
    "a5","b5","c5","d5","e5","f5","g5","h5","--","--","--","--","--","--","--","--",
    "a4","b4","c4","d4","e4","f4","g4","h4","--","--","--","--","--","--","--","--",
    "a3","b3","c3","d3","e3","f3","g3","h3","--","--","--","--","--","--","--","--",
    "a2","b2","c2","d2","e2","f2","g2","h2","--","--","--","--","--","--","--","--",
    "a1","b1","c1","d1","e1","f1","g1","h1","--","--","--","--","--","--","--","--",
};

struct move {
    int8_t start;
    int8_t end;
    int8_t flag;

    void print(){
        

        std::cout << squareName[start]; 
        std::cout << squareName[end];
        std::cout << std::endl;
    }
};

struct board {
    int8_t squares[128];
    int kings[2]; 
    int enpassant;
    bool shortCastle[2];
    bool longCastle[2];
    bool whiteToMove;
    bool inCheck;

    std::vector<move> GeneratesMoves(bool capturesOnly=false){
        std::vector<move> moves = {};

        int advance = N * color[whiteToMove];

        for (int i=0; i<128; i++) {
            if (squares[i] == 0) continue;
            if (squares[i] * color[whiteToMove] <= 0) continue;
            
            int8_t piecetype = abs(squares[i]);
            switch(piecetype){
                case PAWN:
                    addPawnMove(&moves, i, i+advance+W, true);
                    addPawnMove(&moves, i, i+advance+E, true);
                    if (squares[i+advance] == EMPTY) {
                        addPawnMove(&moves, i, i+advance, capturesOnly);
                        if (squares[i+advance+advance] == EMPTY && isHomeRow(i)) {
                            addMove(&moves, i, i+advance+advance, capturesOnly, DOUBLEPUSH);
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
                    addMove(&moves, origin, enpassant, capturesOnly, ENPASSANT);
                }
            }
        }

        int kingIdx = kings[whiteToMove];
        bool canShortCastle = shortCastle[whiteToMove];
        bool canLongCastle = longCastle[whiteToMove];
        bool inCheck = attacked(kingIdx);

        if (!capturesOnly && !inCheck) {
            if (canShortCastle && squares[kingIdx + E] == EMPTY && squares[kingIdx + E + E] == EMPTY && !attacked(kingIdx + E) && !attacked(kingIdx + E + E)) {
                addMove(&moves, kingIdx, kingIdx+E+E, false, CASTLESHORT);
            }
            if (canLongCastle && squares[kingIdx + W] == EMPTY && squares[kingIdx + W + W] == EMPTY && squares[kingIdx + W + W + W] == EMPTY && !attacked(kingIdx + W) && !attacked(kingIdx + W + W)) {
                addMove(&moves, kingIdx, kingIdx+W+W, false, CASTLELONG);
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

    bool valid(int index) {
        return (index & 0x88) == 0;
    }

    void addPawnMove(std::vector<move>* moves, int start, int end, bool capturesOnly) {
        if (isPromotionRow(end)) {
            for (int8_t promotion : std::vector<int8_t>{PROMOTEQUEEN, PROMOTEROOK, PROMOTEKNIGHT, PROMOTEBISHOP}) {
                addMove(moves, start, end, capturesOnly, promotion);
            }
            return;
        }
        addMove(moves, start, end, capturesOnly);
    }

    void addMove(std::vector<move>* moves, int8_t start, int8_t end, bool capturesOnly, int8_t flag=BASIC) {
        if (!valid(end)) return;
        if (capturesOnly && squares[end] == EMPTY) return;

        if (squares[start] * squares[end] <= 0) {
            //std::cout << squareName[start] << squareName[end] << std::endl;
            moves->push_back(move{start,end,flag});
        }
    }

    bool isHomeRow(int index) {
        int rank = index >> 4;
        if (whiteToMove) {
            return rank == 6;
        } else {
            return rank == 1;
        }
    }

    bool isPromotionRow(int index) {
        int rank = index >> 4;
        if (whiteToMove) {
            return rank == 0;
        } else {
            return rank == 7;
        }
    }

    bool attacked(int index) {
        int attackerColor = color[!whiteToMove];
        std::vector<int> pawnAttacks = { S+W, S+E };
        if (whiteToMove) {
            pawnAttacks = { N+W,N+E };
        }
        
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
                if (piece == find) {
                    return true;
                }
                if (!ray || piece != 0) {
                    break;
                }
            }
        }
        return false;
    }

    void edit(int index, int8_t piece){
        squares[index] = piece;
    }

    void print(){
        const std::string pieceChars = "kqrbnp.PNBRQK";
        for (int i=0;i<128;i++){
            if (i%16 == 15) std::cout << std::endl;
            if (i%16 < 8) std::cout << pieceChars[squares[i]+6];
        }
    }
};

board* apply(board* oldBoard, move m){
        board* cBoard = new board;
        if (cBoard == nullptr) {
            //TODO: deallocate if it turns out move is illegal
            std::cout << "nullpointerino" << std::endl;
        }
        std::memcpy(cBoard, oldBoard, sizeof(*oldBoard));

        int8_t movingPiece = cBoard->squares[m.start];

        cBoard->edit(m.end, movingPiece);
        cBoard->edit(m.start, EMPTY);

        bool isWhite = cBoard->whiteToMove;

        if (abs(movingPiece) == KING) {
            cBoard->kings[isWhite] = m.end;
            cBoard->longCastle[isWhite] = false;
            cBoard->shortCastle[isWhite] = false;
        }

        cBoard->enpassant = 0;

        if (m.flag != 0) {
            switch (m.flag) {
                case PROMOTEKNIGHT:
                    cBoard->edit(m.end, KNIGHT * color[isWhite]);
                    break;
                case PROMOTEBISHOP:
                    cBoard->edit(m.end, BISHOP * color[isWhite]);
                    break;
                case PROMOTEROOK:
                    cBoard->edit(m.end, ROOK * color[isWhite]);
                    break;
                case PROMOTEQUEEN:
                    cBoard->edit(m.end, QUEEN * color[isWhite]);
                    break;
                case ENPASSANT:
                    cBoard->edit(m.end + (S*color[isWhite]), EMPTY);
                    break;
                case DOUBLEPUSH:
                    cBoard->enpassant = m.end + (S*color[isWhite]);
                    break;
                case CASTLESHORT:
                    cBoard->edit(m.end + E, EMPTY);
                    cBoard->edit(m.end + W, ROOK*color[isWhite]);
                    break;
                case CASTLELONG:
                    cBoard->edit(m.end + W + W, EMPTY);
                    cBoard->edit(m.end + E, ROOK*color[isWhite]);
                    break;
            }
        }

        if (cBoard->attacked(cBoard->kings[isWhite])) {
            return nullptr;
        }

        if (m.start == H1 || m.end == H1) cBoard->shortCastle[1] = false;
        if (m.start == H8 || m.end == H8) cBoard->shortCastle[0] = false;
        if (m.start == A1 || m.end == A1) cBoard->longCastle[1] = false;
        if (m.start == A8 || m.end == A8) cBoard->longCastle[0] = false;

        cBoard->whiteToMove = !cBoard->whiteToMove;  
        return cBoard;   
    }

std::string beforeWord(std::string str, std::string word) {
    size_t found = str.find(word);
    if (found != std::string::npos) {
        return str.substr(0, found);
    }
    return "";
}

std::string afterWord(std::string str, std::string word) {
    size_t found = str.find(word);
    if (found != std::string::npos) {
        return str.substr(found + word.length());
    }
    return "";
}   

board newBoard(std::string fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
    const std::map<char, int8_t> charToPiece = {
        {'P',PAWN}, {'p',-PAWN},
        {'N',KNIGHT}, {'n',-KNIGHT},
        {'B',BISHOP}, {'b',-BISHOP},
        {'R',ROOK}, {'r',-ROOK},
        {'Q',QUEEN}, {'q',-QUEEN},
        {'K',KING}, {'k',-KING}
    };
    const std::map<char, int8_t> charToOffset = {
        {'/', 8},
        {'1', 1},
        {'2', 2},
        {'3', 3},
        {'4', 4},
        {'5', 5},
        {'6', 6},
        {'7', 7},
        {'8', 8},
    };
    board b = board{};    

    std::string position = beforeWord(fen, " ");

    int8_t i = 0;
    for (char pchar : position) {
        if (charToOffset.count(pchar)) {
            i += charToOffset.at(pchar);
        } else {
            b.edit(i,charToPiece.at(pchar));
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

    return b;        int subtreeNodes = 0;

}

// TODO: make this not leak mem like crazy
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

int main(){
    printf("Calico!\n");

    board perfBoard = newBoard();

    perfBoard.GeneratesMoves();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << perft(&perfBoard, 6) << std::endl;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();

    std::cout << ms_int << std::endl;
}

