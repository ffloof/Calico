int weights[7] = {0,100,280,320,500,900,0};

int evaluate(board* b) {
    int score = 0;
    for(int i=0;i<128;i++){
        int8_t piece = b->squares[i];
        if(piece == EMPTY) continue;
        int value = weights[abs(piece)];
        if (piece > 0) score += value;
        else score -= value;
    }

    return score * color[b->whiteToMove];
}