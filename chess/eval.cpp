
int phases[14] = {0,0,0,0,2,2,2,2,3,3,8,8,0};
int earlyPieces[7] = {0, 100, 400, 440, 575, 1200, 0};
int latePieces[7]  = {0, 100, 290, 320, 550, 1000, 0};

int earlyTable[7][64] = {
    {},
    { // Pawn
  0,   0,   0,   0,   0,   0,   0,   0,
 155, 170, 120, 125, 120, 145, 110,  95,
  45,  60,  60,  55,  65,  60,  60,  35,
  10,  20,  10,  10,  15,   5,  15,  -5,
 -10,   0,   0,   5,  10,  -5,   5, -15,
 -15,   0,  -5,  -5,  -5,  -5,  15, -15,
 -15,   0, -10, -10, -15,  10,  20, -15,
   0,   0,   0,   0,   0,   0,   0,   0,
    }, 
    { // Knight
-130, -75, -30, -45,  15, -70, -45,-120,
 -55, -30,  25,  15,   5,  20, -10, -40,
 -40,  20,  25,  40,  45,  65,  30,   0,
 -15,  10,  20,  40,  30,  45,  10,   0,
 -20,  -5,  15,  20,  20,  20,  10, -15,
 -25, -10, -10,  10,  15,   5,   0, -25,
 -40, -40, -15, -10,  -5,  -5, -20, -35,
 -80, -30, -45, -30, -25, -30, -40, -50,
    },
    { // Bishop
 -25, -10, -55, -25, -20, -30,  -5, -20,
 -20,   5,  -5, -15,  10,  25,   5, -35,
 -10,  15,  20,  20,  15,  30,  20,   0,
  -5,   5,  15,  30,  25,  25,   5,   0,
 -10,   5,  10,  20,  20,  10,   0,  -5,
 -10,   5,  10,  10,  10,  15,   5,  -5,
  -5,   0,   0,   0,   5,   5,  10, -15,
 -35, -10, -20, -15, -15, -15, -25, -25,
    },
    { // Rook
  20,  25,  25,  35,  40,  10,  20,  25,
  20,  20,  35,  40,  40,  35,  15,  25,
   0,  10,  15,  20,  10,  20,  30,   5,
 -15,  -5,  10,  10,  10,  20,  -5, -10,
 -20, -15,  -5,   0,   0, -10,  -5, -20,
 -30, -15, -15, -10,  -5, -10, -10, -30,
 -30, -15, -15,  -5,  -5,   0, -10, -45,
 -15,  -5,   0,   5,   5,  -5, -20, -30,
    },
    { // Queen
-20,  10,  25,  20,  45,  35,  25,  35,
 -25, -10,  10,  20,  20,  45,  30,  30,
 -20,  -5,   5,  30,  40,  50,  35,  35,
 -15,  -5,   0,  15,  30,  30,  30,  20,
 -15,   0,   5,  20,  15,  15,  20,  10,
 -20, -15,   0,   0,   0,  10,  10,   5,
 -35, -20, -10, -10,  -5,  -5, -25, -20,
 -20, -30, -20, -20, -15, -35, -30, -55,
    },
    { // King
 -75,  25,  15, -20, -65, -40,   0,  10,
  30,  -5, -25, -10, -10,  -5, -45, -35,
 -10,  25,   0, -20, -25,   5,  20, -25,
 -20, -25, -15, -30, -35, -30, -15, -40,
 -55,  -5, -30, -45, -55, -50, -40, -60,
 -15, -15, -25, -55, -50, -35, -20, -30,
   0,   5, -10, -75, -50, -20,  10,   5,
 -20,  40,  10, -65,   5, -35,  25,  15,
    }
};

int lateTable[7][64] = {
    {},
    {},
    {},
    {},
    {},
    {},
    {
-85, -40, -20, -20, -15,  15,   0, -20,
 -15,  15,  15,  15,  15,  40,  25,  10,
  10,  15,  25,  15,  20,  50,  45,  10,
 -10,  20,  25,  30,  25,  35,  25,   0,
 -20,  -5,  20,  25,  30,  25,  10, -15,
 -25,  -5,  10,  20,  25,  15,   5, -10,
 -30, -15,   0,  10,  15,   0,  -5, -20,
 -60, -40, -25, -15, -35, -15, -30, -50,
    },
};

int earlyPST[14][128] = {};
int latePST[14][128] = {};

void initPSQT(){
    for(int x=1;x<6;x++){
        for(int y=0;y<64;y++) {
            lateTable[x][y] = earlyTable[x][y];
        }
    }

    for(int piecetype=0;piecetype<=6;piecetype++) {
        for(int i=0;i<64;i++){
            int j = i^56;
            int idx_i = i + (i & ~7);
            int idx_j = j + (j & ~7);
            earlyPST[(piecetype*2)+1][idx_i] = earlyTable[piecetype][i] + earlyPieces[piecetype];
            latePST[(piecetype*2)+1][idx_i] = lateTable[piecetype][i] + latePieces[piecetype];
            earlyPST[(piecetype*2)][idx_i] = -(earlyTable[piecetype][j] + earlyPieces[piecetype]);
            latePST[(piecetype*2)][idx_i] = -(lateTable[piecetype][j] + latePieces[piecetype]);
        }
    }
}

void board::updateEval(int index, int8_t oldPiece, int8_t newPiece) {
    earlyScore -= earlyPST[oldPiece][index];
    lateScore -= latePST[oldPiece][index];
    phase -= phases[oldPiece];
    earlyScore += earlyPST[newPiece][index];
    lateScore += latePST[newPiece][index];
    phase += phases[newPiece];
}

int evaluate(board* b) {
    int truePhase = b->phase;
    if (truePhase > 44) truePhase = 44;
    int score = ((truePhase * b->earlyScore) + ((44-truePhase)*b->lateScore))/44; 

    score += -b->mobilities[0] + b->mobilities[1];

    if (!b->whiteToMove) score = -score;
    return score + 10; // tempobonus
}