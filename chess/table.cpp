uint64_t zobrist[16][128];
uint64_t zobristEP[128];
uint64_t zobristSTM[2];
uint64_t zobristCastleS[2];
uint64_t zobristCastleL[2];

std::random_device rd;
std::mt19937_64 mt(rd());
std::uniform_int_distribution<uint64_t> rng;

void initZobrists(){
    

    for(int x=1;x<16;x++){
        for(int y=0;y<128;y++) zobrist[x][y] = rng(mt);
    }

    for(int y=0;y<128;y++) zobristEP[y] = rng(mt);
    for(int y=0;y<2;y++) {
        zobristSTM[y] = rng(mt);
        zobristCastleS[y] = rng(mt);
        zobristCastleL[y] = rng(mt);
    }
}

uint64_t board::getHash(){
    // We add to the hash the sidetomove, enpassant, and castling rights
    // You could also try to keep these updated incrementally but I am too lazy
    uint64_t finalhash = hash ^ zobristEP[enpassant] ^ zobristSTM[whiteToMove];
    if (shortCastle[0]) finalhash ^= zobristCastleS[0];
    if (shortCastle[1]) finalhash ^= zobristCastleS[1];
    if (longCastle[0]) finalhash ^= zobristCastleL[0];
    if (longCastle[1]) finalhash ^= zobristCastleL[1];


    return finalhash;
}

void board::updateHash(int index, int8_t oldPiece, int8_t newPiece){
    hash ^= zobrist[oldPiece][index];
    hash ^= zobrist[newPiece][index];
}

struct ttentry {
    uint64_t fullHash;
    int16_t score;
    int16_t depth; 
    move tableMove;
    int8_t bound;
};

const int8_t LOWERBOUND = -1, EXACT = 0, UPPERBOUND = 1;

const int tsize = 256000000 / 16; // Each entry is 16 bytes, so 256MB should be div by 16 

ttentry ttable[tsize] = {};

ttentry* tableget(uint64_t key) {
    ttentry* e = &ttable[key % tsize];
    if (e->fullHash == key) return e;
    return nullptr;
}

void tableset(uint64_t key, move m, int16_t depth, int16_t score, int8_t bound) {
    ttentry* e = &ttable[key % tsize];
    ttable[key % tsize] = ttentry{key, score, depth, m, bound};
}

void printpv(board* b){
    std::cout << "pv";
    int i = 0;
    while (b != nullptr) {
        ttentry* entry = tableget(b->getHash());
        if (entry == nullptr) break;
        if (entry->depth <= 0 && (entry->tableMove.start == entry->tableMove.end)) break;
        std::cout << " ";
        entry->tableMove.print(); 
        b = apply(b, entry->tableMove);
        i += 1;
        if(i > 40) break;
    }
}