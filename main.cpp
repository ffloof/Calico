#include "chess/board.h"
#include <iostream>
#include <chrono>

int main(){
    printf("Calico!\n");
    board perfBoard = newBoard();
    

    perfBoard.GeneratesMoves();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << perft(&perfBoard, 6) << std::endl;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)).count();

    std::cout << ms_int << std::endl;
}

