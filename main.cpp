#include "chess/board.cpp"
#include "chess/table.cpp"
#include "chess/eval.cpp"
#include "chess/search.cpp"
#include <iostream>
#include <chrono>

board* unrollMoveStr(board* b,std::string remainingMoves){
    if (remainingMoves == "") return b;
    b = applyMoveStr(b, beforeWord(remainingMoves, " "));
    return unrollMoveStr(b, afterWord(remainingMoves, " "));
}

int main(){
    initZobrists();
    initPSQT();
    
    std::string line;
    board uciBoard = newBoard(); 

    while (std::getline(std::cin, line)) {
        std::string command = beforeWord(line, " ");
        std::string arguments = afterWord(line, " ");

        if (command=="uci"){
            std::cout << "id name calico" << std::endl;
            std::cout << "id author ffloof" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok";
        } else if (command == "print") {
            uciBoard.print();
        } else if (command == "perft") {
            auto t1 = std::chrono::high_resolution_clock::now();
            std::cout << perft(&uciBoard, std::stoi(arguments)) << std::endl;
            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count() << std::endl;
        } else if (command == "position") {
            if (beforeWord(arguments, "moves") == "startpos"){
                uciBoard = newBoard();
            } else {
                uciBoard = newBoard(afterWord(beforeWord(arguments, "moves"), " "));
            }
            uciBoard = *(unrollMoveStr(&uciBoard, afterWord(arguments, "moves")));
        } else if (command == "go") {
            int totalTime;
            if (uciBoard.whiteToMove) totalTime = std::stoi(beforeWord(afterWord(arguments, "wtime"), " "));
            else totalTime = std::stoi(beforeWord(afterWord(arguments, "btime"), " "));
            move chosenMove = iterativeSearch(&uciBoard, totalTime/30);
            std::cout << "bestmove ";
            chosenMove.print();
        }
    }
}

