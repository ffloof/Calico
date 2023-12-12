#include "chess/board.cpp"
#include "chess/table.cpp"
#include "chess/eval.cpp"
#include "chess/search.cpp"
#include <iostream>
#include <chrono>
#include <fstream>

board uciBoard = newBoard();

std::vector<unsigned long long> unrollMoveStr(board* b,std::string remainingMoves){
    if (remainingMoves == "") { 
        uciBoard = *b;
        std::vector<unsigned long long> posHistory = {};
        return posHistory;
    }
    b = applyMoveStr(b, beforeWord(remainingMoves, " "));
    std::vector<unsigned long long> posHistory = unrollMoveStr(b, afterWord(remainingMoves, " "));
    if (posHistory.size() < 12) posHistory.push_back(b->getHash());
    delete b;

    return posHistory;
}

int main(){
    std::vector<unsigned long long> prevPositions;
    initZobrists();
    initPSQT();
    
    std::string line;

    std::ofstream outputFile("inputlog.txt");
    outputFile << "START" << std::endl;

    while (std::getline(std::cin, line)) {
        outputFile << line << std::endl;
        std::string command = beforeWord(line, " ");
        std::string arguments = afterWord(line, " ");

        if (command=="uci"){
            std::cout << "id name Calico" << std::endl;
            std::cout << "id author ffloof" << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
            std::cout << "option name Hash type spin default 1 min 1 max 1" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "quit") {
            return 0;
        } else if (command == "print") {
            uciBoard.print();
        } else if (command == "perft") {
            auto t1 = std::chrono::high_resolution_clock::now();
            std::cout << perft(&uciBoard, std::stoi(arguments)) << std::endl;
            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count() << "ms" << std::endl;
        } else if (command == "position") {
            if (beforeWord(arguments, "moves") == "startpos"){
                uciBoard = newBoard();
            } else {
                uciBoard = newBoard(afterWord(beforeWord(arguments, "moves"), " "));
            }
            prevPositions = unrollMoveStr(&uciBoard, afterWord(arguments, "moves"));
        } else if (command == "go") {
            int totalTime;
            try {
                if (uciBoard.whiteToMove) totalTime = std::stoi(beforeWord(afterWord(arguments, "wtime"), " "));
                else totalTime = std::stoi(beforeWord(afterWord(arguments, "btime"), " "));
                iterativeSearch(&uciBoard, totalTime/50, prevPositions);
            } catch(const std::invalid_argument& e) {
                try {
                    totalTime = std::stoi(beforeWord(afterWord(arguments, "movetime"), " "));
                    iterativeSearch(&uciBoard, totalTime/2, prevPositions);
                } catch(const std::invalid_argument& e) {
                    iterativeSearch(&uciBoard, 10000, prevPositions);
                }
            }
        }
    }
}

