#include "raylib.h"
#include <vector>
#include <iostream>
#include <string>

#include "Renderer.h"
#include "Precomputation.h"
#include "MoveGenerator.h"
#include "Perft.h"


int main()
{
	const int width = 1600;
	const int height = 1200;

	Renderer renderer(width, height);
	BoardState board{};
	board.parseFEN("k5q1/8/8/3pP3/8/1K6/8/8 w - - 0 1");

	MoveGenerator moveGenerator;
	
	MoveArr movesArr{};
	int moveCount = moveGenerator.generateLegalMoves(movesArr, board);

	std::cout << "Move Count: " << moveCount << "\n";

	//renderer.visualizeBoard(moveGenerator.cashedPinD12);

	//renderer.visualizeBoard(moveGenerator.generateHVPinMask(board.all(), board.white(), board.blackRooks | board.blackQueens, board.whiteKing));

	const int depth = 5;
	Timer timer;
	timer.start();
	uint64_t perftResults = perft(depth, moveGenerator, board);
	timer.stop();

	std::cout << "Depth - " << depth << " - Time: " << timer.elapsedTime<std::chrono::milliseconds>() << '\n';
	std::cout << "Move count: " << (int)perftResults << '\n';

	int i = 0;
	SetTargetFPS(2);
	while (!renderer.shouldClose())
	{
		board.makeMove(movesArr[i % (moveCount-1)]);
		renderer.update(board);
		board.unmakeMove();
		++i;
	}
	
	return EXIT_SUCCESS;
}