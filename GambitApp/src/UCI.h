#pragma once
#include "Board.h"
#include "MoveGenerator.h"
#include "Search.h"
#include <string>

class UCI {
public:
    static void loop();
    static void processCommand(const std::string& command);
    static void setupPosition(const std::string& fen, const std::vector<std::string>& moves);
    static void startSearch(const std::string& parameters);
    static void printBoard(const BoardState& board);

private:
    static BoardState board;
    static MoveGenerator moveGen;
    static bool uciMode;
    static bool debugMode;
    static Searcher searcher;
};
		
