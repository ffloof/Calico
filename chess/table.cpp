uint64_t zobrist[16][128];
uint64_t zobristEP[128];
uint64_t zobristSTM[2];
uint64_t zobristCastleS[2][2];
uint64_t zobristCastleL[2][2];

void initZobrists(){
    std::random_device rd;
    std::mt19937_64 mt(rd());
    std::uniform_int_distribution<uint64_t> rng;

    for(int x=1;x<16;x++){
        for(int y=0;y<128;y++) zobrist[x][y] = rng(mt);
    }

    for(int y=0;y<128;y++) zobristEP[y] = rng(mt);
    for(int y=0;y<2;y++) zobristSTM[y] = rng(mt);

    for(int x=0;x<2;x++) {
        for(int y=0;y<2;y++) zobristCastleS[y][x] = rng(mt);
        for(int y=0;y<2;y++) zobristCastleL[y][x] = rng(mt);
    }
}

uint64_t board::getHash(){
    // We add to the hash the sidetomove, enpassant, and castling rights
    // You could also try to keep these updated incrementally but I am too lazy
    return (hash ^ zobristEP[enpassant] ^ zobristSTM[whiteToMove] ^ zobristCastleS[0][shortCastle[0]] ^ zobristCastleS[1][shortCastle[1]] ^ zobristCastleL[0][longCastle[0]] ^ zobristCastleL[1][longCastle[1]]);
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
    ttable[key % tsize] = ttentry{key, score, depth, m, bound};
}

void tablesetempty(uint64_t key, move m, int16_t score, int8_t bound) {
    ttentry* e = &ttable[key % tsize];
    if (e->depth <= 1) {
        ttable[key % tsize] = ttentry{key, score, 0, m, bound};
    }
}

void printpv(board* b){
    std::cout << "pv";
    int i = 0;
    while (b != nullptr) {
        ttentry* entry = tableget(b->getHash());
        if (entry == nullptr) break;
        if (entry->depth <= 0 && entry->tableMove.start == 0 && entry->tableMove.end ==0 ) break;
        std::cout << " ";
        entry->tableMove.print(); 
        b = apply(b, entry->tableMove);
        i += 1;
        if(i > 40) break;
    }
}