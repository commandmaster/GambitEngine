#pragma once

#include <iostream>

#include "Board.h"
#include "Search.h"

void testDepth(int depth)
{
	Searcher searcher;
	BoardState board;

	board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	Timer timer;
	timer.start();
	searcher.findBestMove(board, depth, 10000);
	timer.stop();
	
	std::cout << std::dec << timer.elapsedTime<std::chrono::milliseconds>() << "\n";
}