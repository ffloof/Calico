import sys
import chess
import numpy as np
import random
from tqdm import tqdm

dataPath = sys.argv[1]

file1 = open(dataPath, 'r')

lines = []

for line in file1.readlines():
    lines.append(line)

random.shuffle(lines)

outcomescore = {
    "1-0":1,
    "0-1":0,
    "1/2-1/2":0.5,
}

sign = [-1,1]
phase_level = [0,0,2,2,3,8,0] # sum 44

inputs = []
outputs = []

N = -10
S = 10
W = -1
E = 1

mailbox = [
    21, 22, 23, 24, 25, 26, 27, 28,
    31, 32, 33, 34, 35, 36, 37, 38,
    41, 42, 43, 44, 45, 46, 47, 48,
    51, 52, 53, 54, 55, 56, 57, 58,
    61, 62, 63, 64, 65, 66, 67, 68,
    71, 72, 73, 74, 75, 76, 77, 78,
    81, 82, 83, 84, 85, 86, 87, 88,
    91, 92, 93, 94, 95, 96, 97, 98
]

rays = [ False, False, False, True, True, True, False]
patterns = [ [], [], [N+N+W,N+N+E,S+S+W,S+S+E,W+W+N,W+W+S,E+E+N,E+E+S], [N+W,N+E,S+W,S+E], [N,S,E,W], [N,S,E,W,N+W,N+E,S+W,S+E], [N,S,E,W,N+W,N+E,S+W,S+E]]

for line in tqdm(lines):
    if len(outputs) > 500000:
        break

    packed = line.split("c9")
    fen = packed[0].strip()
    outcome = outcomescore[packed[1].strip().replace(";", "").replace("\"", "")]
    board = chess.Board(fen)
    
    phase = 0
    material = np.zeros(7)
    location = np.zeros((7,64))
    pawns = np.zeros((2,10))

    virtualboard = [
        1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,
    ]

    for i in range(64):
        piece = board.piece_at(i)
        if piece == None:
            continue

        virtualboard[mailbox[i]] = (piece.piece_type * 2) + piece.color

        phase += phase_level[piece.piece_type]

        material[piece.piece_type] += sign[piece.color]
        j = i
        if piece.color:
            j = i ^ 56

        location[piece.piece_type][j] += sign[piece.color]

        if piece.piece_type == 1: # (pawn)
            pawns[int(piece.color)][(j%8)+1] += 1

    attackers = np.zeros((14,14)) # TODO: add another table of attackers with tempo
    mobilities = np.zeros(7)

    for sq in range(len(virtualboard)):
        piece = virtualboard[sq]
        piecetype = piece // 2
        isray = rays[piecetype]
        pattern = patterns[piecetype]

        if piece == 2:
            pattern = [S+W,S+E]
        elif piece == 3:
            pattern = [N+W,N+E]

        mobility = 0
        for direction in pattern:
            current = sq
            for i in range(1,8):
                current += direction
                if virtualboard[current] != 0:
                    attackers[piece][virtualboard[current]] += 1
                    break
                mobility += 1
                if not isray:
                    break
        mobilities[piecetype] += mobility * sign[piece & 1]

    isolated_pawns = np.zeros(10)
    fullpass_pawns = np.zeros(10)
    semiopen_files = np.zeros(10)

    for file in range(1,9):
        if pawns[0][file-1] == 0 and pawns[0][file+1] == 0:
            isolated_pawns[file] -= pawns[0][file]
            if pawns[0][file] == 0 and pawns[1][file] != 0:
                fullpass_pawns[file] += 1 
        if pawns[1][file-1] == 0 and pawns[1][file+1] == 0:
            isolated_pawns[file] += pawns[1][file]
            if pawns[1][file] == 0 and pawns[0][file] != 0:
                fullpass_pawns[file] -= 1 

        if pawns[0][file] == 0:
            semiopen_files[file] -= 1
        if pawns[1][file] == 0:
            semiopen_files[file] += 1

    sidetomove = np.zeros(1)
    sidetomove[0] = sign[board.turn]

    if phase > 44:
        phase = 44
    
    start_weight = phase / 44
    end_weight = (44 - phase) / 44

    values = np.concatenate([material, location.flatten(), isolated_pawns, fullpass_pawns, sidetomove, mobilities, attackers.flatten()])
    tapered_values = np.concatenate([values * start_weight, values * end_weight])

    inputs.append(tapered_values)
    outputs.append(outcome)

    #print(fen)
    #print("isolated", isolated_pawns)
    #print("open", semiopen_files)
    #print("passed", fullpass_pawns)
    #print(mobilities)
    #print(attackers)
    #sys.exit()




from sklearn.neural_network import MLPRegressor

mlp_regressor = MLPRegressor(hidden_layer_sizes=(1,), activation='logistic', verbose=True)

# Fit the MLPRegressor to your data
mlp_regressor.fit(inputs, outputs)

# Accessing the coefficients and intercept of the single-layer perceptron model
weights = mlp_regressor.coefs_[0]
bias = mlp_regressor.intercepts_[0]

half = len(weights)//2

def printcomma(idx):
    n1 = weights[idx][0]
    n2 = weights[idx + half][0]
    #print("{:.3f}".format(n), end=",")
    print("S("+str(int(128 * n1)) + "," + str(int(128*n2)) +")", end=",")

def printflat(start,end):
    for y in range(start,end):
        printcomma(y)
    print("")

def printpsqt(start,end,size): # m allows for adjusting psqt by material
    x = weights[start:end]
    i = 0
    for y in range(start,end):
        printcomma(y)
        if i % size == size-1:
            print("")
        i += 1



print("Bias:", bias)

print("Material")
printflat(0,7)

piece = ["empty","pawn","knight","bishop","rook","queen","king"]

for a in range(7):
    piecename = piece[a]
    print("\npsqt", piecename)
    printpsqt(7+64*a,7+64*(a+1),size=8)

print("Isolated")
printflat(7*64+7,7*64+17)

print("Passed")
printflat(7*64+17,7*64+27)

print("Tempo")
printflat(7*64+27,7*64+28)

print("Mobility")
printflat(7*64+28,7*64+35)

print("Attackers")
printpsqt(7*64+35,7*64+35+(14*14),size=14)

print("Loss curve:", mlp_regressor.loss_curve_)

y_pred = mlp_regressor.predict(inputs)

residuals = outputs - y_pred

index_largest_residual = np.argmax(np.abs(residuals))
print("Largest residual:", outputs[index_largest_residual], y_pred[index_largest_residual], residuals[index_largest_residual], lines[index_largest_residual])

print("farout", np.sum(y_pred[y_pred < -0.1]) + np.sum(y_pred[y_pred > 1.1]))
print("r^2", mlp_regressor.score(inputs, outputs))