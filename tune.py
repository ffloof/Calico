import sys
import chess
import numpy as np
import random
from tqdm import tqdm
import matplotlib.pyplot as plt

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
    if len(outputs) > 400000:
        break

    packed = line.split("c9")
    fen = packed[0].strip()
    outcome = outcomescore[packed[1].strip().replace(";", "").replace("\"", "")]
    board = chess.Board(fen)
    
    phase = 0
    material = np.zeros(7)

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

    kings = [-1, -1]
    

    for i in range(64):
        piece = board.piece_at(i)
        if piece == None:
            continue

        if piece.piece_type == 6:
            kings[piece.color] = ((i & 7) < 4)


    dynamicpawns = np.zeros((4,64))

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

        if piece.piece_type == 1:
            pawnSide = ((i & 7) < 4)
            sameWhiteKing = int(pawnSide == kings[0])
            sameBlackKing = int(pawnSide == kings[1])

            whitetypeindex = sameWhiteKing * 2 + sameBlackKing
            blacktypeindex = sameBlackKing * 2 + sameWhiteKing

            if pawnSide:
                j = j ^ 7
            
            typeindex = blacktypeindex
            if piece.color:
                typeindex = whitetypeindex

            dynamicpawns[typeindex, j] += sign[piece.color]

    dynamicpawns = dynamicpawns.reshape((4,8,8))
    dynamicpawns = dynamicpawns[:,:,4:]
    dynamicpawns.reshape((4,32))

    #print(dynamicpawns)
    #print(kings)
    #print(fen)
    #sys.exit()






    #attackers = np.zeros((14,14)) # TODO: add another table of attackers with tempo
    domesticmobilities = np.zeros(7)
    foreignmobilities = np.zeros(7)

    for sq in range(len(virtualboard)):
        piece = virtualboard[sq]
        piecetype = piece // 2
        isray = rays[piecetype]
        pattern = patterns[piecetype]

        if piece == 2:
            #pattern = [S+W,S+E]
            pattern = []
        elif piece == 3:
            #pattern = [N+W,N+E]
            pattern = []
        
        domesticmobility = 0
        foreignmobility = 0

        for direction in pattern:
            current = sq
            for i in range(1,8):
                current += direction

                domestic = (current >= 60)
                if piece & 1 == 1:
                    domestic = not domestic

                if virtualboard[current] != 0:
                    if ((piece & 1) == (virtualboard[current] & 1)) or virtualboard[current] == 1:
                        break

                    if domestic:
                        domesticmobility += 1
                    else:
                        foreignmobility += 1
                    break

                if not isray:
                    break
        
        #if virtualboard[sq] > 1:
            #print("  pPnNbBrRqQkK"[virtualboard[sq]] , domesticmobility * sign[piece & 1], foreignmobility * sign[piece & 1])
        domesticmobilities[piecetype] += domesticmobility * sign[piece & 1]
        foreignmobilities[piecetype] += foreignmobility * sign[piece & 1]


    sidetomove = np.zeros(1)
    sidetomove[0] = sign[board.turn]


    
    if phase > 44:
        phase = 44
    
    start_weight = phase / 44
    end_weight = (44 - phase) / 44

    values = np.concatenate([material, domesticmobilities, foreignmobilities, sidetomove, dynamicpawns.flatten()])
    tapered_values = np.concatenate([values * start_weight, values * end_weight])

    inputs.append(tapered_values)
    outputs.append(outcome)

    '''
    print(fen)

    print(material)
    print(domesticmobilities)
    print(foreignmobilities)
    sys.exit()'''




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

def printpsqt(start,end,size=8): # m allows for adjusting psqt by material
    x = weights[start:end]
    i = 0
    for y in range(start,end):
        printcomma(y)
        if i % size == size-1:
            print("")
        i += 1

def plotpsqt(start, label=""):
    x = weights[start:start+32] * 128
    y = weights[start+half:start+half+32] * 128
    x = x.reshape((8,4))
    y = y.reshape((8,4))
    fig, (ax1, ax2) = plt.subplots(2)
    ax1.imshow(x, vmin=-100, vmax=100)
    ax1.set_title(label + " midgame")
    ax2.imshow(y, vmin=-100, vmax=100)
    ax2.set_title(label + " endgame")
    plt.show()



print("Bias:", bias)
print("Weights:", weights)

print("Loss curve:", mlp_regressor.loss_curve_)

y_pred = mlp_regressor.predict(inputs)

residuals = outputs - y_pred

index_largest_residual = np.argmax(np.abs(residuals))
print("Largest residual:", outputs[index_largest_residual], y_pred[index_largest_residual], residuals[index_largest_residual], lines[index_largest_residual])

print("farout", np.sum(y_pred[y_pred < -0.1]) + np.sum(y_pred[y_pred > 1.1]))
print("r^2", mlp_regressor.score(inputs, outputs))

printflat(0,7)
printflat(7,14)
printflat(14,21)
printflat(21,22)
#printpsqt(22,86)
#printpsqt(86,150)
#printpsqt(150,214)
#printpsqt(214,278)

plotpsqt(22, "Queenside vs Queenside")
plotpsqt(54, "Kingside vs Queenside")
plotpsqt(86, "Queenside vs Kingside")
plotpsqt(118, "Kingside vs Kingside")