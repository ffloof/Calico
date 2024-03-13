import pyautogui as auto
import time
from PIL import ImageGrab
import chess
import chess.engine
import sys
import numpy

thinkTime = 1
thinkNoise = 0.5
touchTime = 0.05
touchNoise = 0.1
moveTime = 0.05
moveNoise = 0.05

side = sys.argv[1].lower()
playingWhite = True
if side == "w" or side == "white" or side == "1":
    playingWhite = True
elif side == "b" or side == "black" or side == "0":
    playingWhite = False 


board = chess.Board()

light = (240, 217, 181)
dark = (181, 136, 99)
light_green = (205, 210, 106)    
dark_green = (170, 162, 58)

def convert(x,y, white):
    files = ["h","g","f","e","d","c","b","a"]
    ranks = ["1","2","3","4","5","6","7","8"]
    if white:
        files = ["a","b","c","d","e","f","g","h"]
        ranks = ["8","7","6","5","4","3","2","1"]
    return files[x] + ranks[y]

def getx(square, white):
    if white:
        return ord(square[0]) - ord('a')
    else:
        return 7 - (ord(square[0]) - ord('a'))

def gety(square, white):
    if white:
        return 7 - (int(square[1]) - 1)
    else:
        return (int(square[1]) - 1)

top_x = 558
top_y = 150
square = 102

engine = chess.engine.SimpleEngine.popen_uci("./a.out")
allowEngine = True

lastmove = ""

while True:
    if (playingWhite == (board.turn == chess.WHITE)):
        if allowEngine:
            allowEngine = False 
            result = str(engine.play(board, chess.engine.Limit(time=(thinkTime + numpy.random.exponential(thinkNoise)))).move)
            print("engine", result)
            chosenstart = result[0:2]
            chosenend = result[2:4]

            move_start_x = top_x + (getx(chosenstart, playingWhite) * square) + (square/2)
            move_start_y = top_y + (gety(chosenstart, playingWhite) * square) + (square/2)
            move_end_x = top_x + (getx(chosenend, playingWhite) * square) + (square/2)
            move_end_y = top_y + (gety(chosenend, playingWhite) * square) + (square/2)

            auto.moveTo(move_start_x, move_start_y, duration=touchTime + numpy.random.exponential(touchNoise), tween=auto.easeInOutQuad)
            auto.click()
            auto.moveTo(move_end_x, move_end_y, duration=moveTime + numpy.random.exponential(moveNoise), tween=auto.easeInOutQuad)
            auto.click()
            if len(result) == 5: # Click again to choose piece to promote to
                time.sleep(0.1)
                auto.click()
    

    # Update internal board state
    screen = ImageGrab.grab(bbox=(top_x, top_y, top_x + (square * 8), top_y + (square * 8)))

    start = []
    end = []
    for y in range(8):
        for x in range(8):
            coord_x = int((square * x))
            coord_y = int((square * y))
            color = screen.getpixel((coord_x, coord_y))
            if color == dark_green or color == light_green:
                center_color = screen.getpixel((coord_x+(square//2), coord_y+(square//2)))
                if center_color == dark_green or center_color == light_green:
                    start.append(convert(x,y,playingWhite))
                else:
                    end.append(convert(x,y,playingWhite))

    move = ""
    if len(start) == 1 and len(end) == 1:
        move = start[0] + end[0]
    elif len(start) == 2: # Castling leaves two start squares on lichess for whatever reason
        if start[0] + start[1] == "e1h1":
            move = "e1g1"
        if start[0] + start[1] == "e8h8":
            move = "e8g8"
        if start[0] + start[1] == "e1a1":
            move = "e1c1"
        if start[0] + start[1] == "e8a8":
            move = "e8c8"
        if start[1] + start[0] == "e1h1":
            move = "e1g1"
        if start[1] + start[0] == "e8h8":
            move = "e8g8"
        if start[1] + start[0] == "e1a1":
            move = "e1c1"
        if start[1] + start[0] == "e8a8":
            move = "e8c8"

    if len(move) == 4:
        if move != lastmove:
            try: 
                if board.piece_at(chess.parse_square(move[0:2])) == None:
                    continue # This typically happens while the piece is in motion and it miss marks the squares
                # TODO: we might not even need this lol, bug was from somewhere else
                if move[3:4] == "8" and "P" == str(board.piece_at(chess.parse_square(move[0:2]))): # Add promotion
                    move += "q"
                elif move[3:4] == "1" and "p" == str(board.piece_at(chess.parse_square(move[0:2]))):
                    move += "q"
                
                board.parse_uci(move)
                lastmove = move
                allowEngine = True
                board.push(chess.Move.from_uci(move))
                print(move, "->", board.fen())
            except:
                print('error')
