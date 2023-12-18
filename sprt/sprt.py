import chess
import chess.engine
import chess.pgn
import datetime
import random
import os
from math import pow, sqrt, log, log10, copysign, pi


engines = ["./NMP", "./RFP"] # [old, new]
old_engine = chess.engine.SimpleEngine.popen_uci(engines[0])
new_engine = chess.engine.SimpleEngine.popen_uci(engines[1])

def game(engineWhite, engineBlack, names, fen='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'):
    print(fen.strip(), end=" moves ")
    board = chess.Board(fen)

    white_time = 10
    black_time = 10
    increment = 0.1

    while not board.is_game_over(claim_draw=True):
        start_t = datetime.datetime.now()
        
        if board.turn == chess.WHITE:
            result = engineWhite.play(board, chess.engine.Limit(white_clock=white_time, black_clock=black_time, white_inc=increment, black_inc=increment))
        else:
            result = engineBlack.play(board, chess.engine.Limit(white_clock=white_time, black_clock=black_time, white_inc=increment, black_inc=increment))
        delta_t = (datetime.datetime.now() - start_t).total_seconds()

        if board.turn == chess.WHITE:
            white_time += increment - delta_t
            if white_time < 0:
                print("WARNING:", names[0], "TIMEOUT", white_time, black_time)
                #return -1
        else:
            black_time += increment - delta_t
            if black_time < 0:
                print("WARNING:", names[1], "TIMEOUT", white_time, black_time)
                #return 1
        if result.move in board.legal_moves:
            print(result.move, end=" ")
            board.push(result.move)
        else:
            print("ILLEGAL MOVE")
            break

    out = board.outcome(claim_draw=True)
    colormap = {chess.WHITE:1, chess.BLACK:-1, None:0}
    print(out.termination)
    print(board)
    print(colormap[out.winner], names, white_time, black_time)
    return colormap[out.winner]

def expected_score(x: float) -> float:
    return 1.0 / (1.0 + pow(10, -x / 400.0))

def gsprt(wins: int,losses: int,draws: int,elo0: float,elo1: float,) -> float:
    p0 = expected_score(elo0)
    p1 = expected_score(elo1)

    N = wins + losses + draws
    if wins == 0 or losses == 0 or draws == 0:
        return 0.0

    w = wins / N
    d = draws / N

    X = w + d / 2
    varX = (w + d / 4 - pow(X, 2)) / N

    return (p1 - p0) * (2 * X - p0 - p1) / (2 * varX)


def erf_inv(x):
    a = 8 * (pi - 3) / (3 * pi * (4 - pi))
    y = log(1 - x * x)
    z = 2 / (pi * a) + y / 2
    return copysign(sqrt(sqrt(z * z - y / a) - z), x)


def phi_inv(p):
    return sqrt(2)*erf_inv(2*p-1)


def elo(score: float) -> float:
    if score <= 0 or score >= 1:
        return 0.0
    return -400 * log10(1 / score - 1)


def elo_wld(wins, losses, draws):
    # win/loss/draw ratio
    N = wins + losses + draws

    p_w = float(wins) / N
    p_l = float(losses) / N
    p_d = float(draws) / N

    mu = p_w + p_d/2
    stdev = sqrt(p_w*(1-mu)**2 + p_l*(0-mu)**2 + p_d*(0.5-mu)**2) / sqrt(N)

    # 95% confidence interval for mu
    mu_min = mu + phi_inv(0.025) * stdev
    mu_max = mu + phi_inv(0.975) * stdev

    return (elo(mu_min), elo(mu), elo(mu_max))

def update_wdl(wdl, result):
    w,d,l = wdl
    if result == 1:
        w += 1
    elif result == 0:
        d += 1
    elif result == -1:
        l += 1
    return (w,d,l)

# Get all pgn/epd files in same directory and parse them
def get_openings():
    fens = []
    for file in os.listdir("./"):
        if file.endswith(".pgn"):
            pgn = open(file)
            while True:
                pgngame = chess.pgn.read_game(pgn)
                if pgngame == None:
                    break
                pgnboard = pgngame.board()
                for move in pgngame.mainline_moves():
                    pgnboard.push(move)
                fens.append(pgnboard.fen())
        elif file.endswith(".epd"):
            fenlist = open(file)
            for line in fenlist:
                fens.append(line)
    return fens

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="llr calculator")
    parser.add_argument('-e0', '--elo0', type=float, help="lower elo", default=0)
    parser.add_argument('-e1', '--elo1', type=float, help="upper elo", default=5)
    parser.add_argument('-a', '--alpha', type=float, help="allowed margin of error for Type 1", default=0.05)
    parser.add_argument('-b', '--beta', type=float, help="allowed margin of error for Type 2",default=0.05)
    args = parser.parse_args()
    
    fens = get_openings()
    print("Fens loaded:", len(fens))
        
    lower = log(args.beta / (1 - args.alpha))
    upper = log((1 - args.beta) / args.alpha)
    wins = 0
    losses = 0
    draws = 0

    while True:
        # Run a new set of games
        fen = random.choice(fens)
        wins, draws, losses = update_wdl((wins, draws, losses), game(new_engine, old_engine, engines[::-1], fen))
        wins, draws, losses = update_wdl((wins, draws, losses), -game(old_engine, new_engine, engines, fen))

        # Compute sprt stuff
        llr = gsprt(wins,losses,draws,args.elo0,args.elo1)
        e1, e2, e3 = elo_wld(wins, losses, draws)
        print(f"ELO: {e2:.3} +- {(e3 - e1) / 2:.3} [{e1:.3}, {e3:.3}]")
        print(f"LLR: {llr:.3} [{args.elo0}, {args.elo1}] ({lower:.3}, {upper:.3})")
        print((wins, draws, losses))

        # Break if we can accept H0 or H1
        if llr >= upper:
            break
        elif llr <= lower:
            break

    print("Complete")
