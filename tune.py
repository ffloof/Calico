import sys
import chess
import numpy as np
import random

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

for line in lines:
    if len(outputs) > 1000000:
        break

    packed = line.split("c9")
    fen = packed[0].strip()
    outcome = outcomescore[packed[1].strip().replace(";", "").replace("\"", "")]
    board = chess.Board(fen)
    
    phase = 0
    material = np.zeros(7)
    location = np.zeros((7,64))

    for i in range(64):
        piece = board.piece_at(i)
        if piece == None:
            continue

        phase += phase_level[piece.piece_type]

        material[piece.piece_type] += sign[piece.color]
        j = i
        if not piece.color:
            j = i ^ 56

        location[piece.piece_type][j] += sign[piece.color]

    if phase > 44:
        phase = 44
    
    start_weight = phase / 44
    end_weight = (44 - phase) / 44


    start_material = material * start_weight
    end_material = material * end_weight

    start_location = location * start_weight
    end_location = location * end_weight

    inputs.append(np.concatenate([start_material, end_material]))
    #inputs.append(np.concatenate([start_material, end_material, start_location.flatten(), end_location.flatten()]))
    outputs.append(outcome)

#print(inputs, outputs)        

from sklearn.neural_network import MLPRegressor

mlp_regressor = MLPRegressor(hidden_layer_sizes=(), activation='logistic')

# Fit the MLPRegressor to your data
mlp_regressor.fit(inputs, outputs)

print(mlp_regressor.score(inputs, outputs))

# Accessing the coefficients and intercept of the single-layer perceptron model
weights = mlp_regressor.coefs_[0]
bias = mlp_regressor.intercepts_[0]

def printflat(x):
    for y in x:
        print("{:.3f}".format(y[0]), end=",")
    print("")

def printpsqt(x, m):
    i = 0
    for y in x:
        print("{:.3f}".format(y[0] + m[0]), end=",")
        if i % 8 == 7:
            print("")
        i += 1

print("Bias:", bias)

print("Early Material")
printflat(weights[0:7])
print("Late Material")
printflat(weights[7:14])

for i in range(14):
    print("\npsqt", i)
    printpsqt(weights[14+64*i:14+64*(i+1)], weights[i])

print(mlp_regressor.loss_curve_)