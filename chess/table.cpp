struct ttentry {
    unsigned long long fullHash;
    int16_t score;
    int16_t depth; 
    move tableMove;
    int8_t bound;
};

const int8_t LOWERBOUND = -1, EXACT = 0, UPPERBOUND = 1;

const int tsize = 10000000;

ttentry ttable[10000000] = {};

ttentry* tableget(unsigned long long key) {
    ttentry* e = &ttable[key % tsize];
    if (e->fullHash == key) return e;
    return nullptr;
}

void tableset(board* b, move m, int16_t depth, int16_t score, int8_t bound) {
    unsigned long long key = b->getHash();
    ttable[key % tsize] = ttentry{key, score, depth, m, bound};
}

void printpv(board* b){
    std::cout << "pv";
    while (b != nullptr) {
        ttentry* entry = tableget(b->getHash());
        if (entry == nullptr) break;
        if (entry->depth <= 0) break;
        std::cout << " ";
        entry->tableMove.print(); 
        b = apply(b, entry->tableMove);
    }
}