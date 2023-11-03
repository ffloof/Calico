#include <string>
#include <vector>

const int color[2] = {-1,1};
const int N = -16, S = 16, W = -1, E = 1;
const int8_t EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
const int8_t BASIC = 0, CASTLELONG = 1, CASTLESHORT = 2, DOUBLEPUSH = 3, ENPASSANT = 4, NULLMOVE = 5, PROMOTEQUEEN = 6, PROMOTEROOK = 7, PROMOTEBISHOP = 8, PROMOTEKNIGHT = 9;
const int A8 = 0, H8 = 7, A1 = 112, H1 = 119; 

struct move {
    int8_t start;
    int8_t end;
    int8_t flag;

    void print();
};

struct board {
    int8_t squares[128];
    int kings[2]; 
    int enpassant;
    bool shortCastle[2];
    bool longCastle[2];
    bool whiteToMove;
    bool inCheck;

    std::vector<move> GeneratesMoves(bool capturesOnly=false);
    void PieceMoves(std::vector<move>* moves, int start, std::vector<int> pattern, bool ray, bool capturesOnly);
    bool valid(int index);
    void addPawnMove(std::vector<move>* moves, int start, int end, bool capturesOnly);
    void addMove(std::vector<move>* moves, int8_t start, int8_t end, bool capturesOnly, int8_t flag=BASIC);
    bool isHomeRow(int index);
    bool isPromotionRow(int index);
    bool attacked(int index);
    bool scan(int8_t find, int start, std::vector<int> pattern, bool ray);
    void edit(int index, int8_t piece);
    void print();
};

board* apply(board* oldBoard, move m);
board newBoard(std::string fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

// TODO: make this not leak mem like crazy
int perft(board* b, int depth);