#include "Perft.h"

uint64_t perft(int depth, MoveGenerator& moveGen, BoardState& board)
{
	MoveArr moveArr;
	int moveCount, i;
	uint64_t nodes = 0;

	if (depth == 0)
		return 1ULL;

	moveCount = moveGen.generateLegalMoves(moveArr, board);
	for (i = 0; i < moveCount; ++i)
	{
		board.makeMove(moveArr[i]);
		nodes += perft(depth - 1, moveGen, board);
		board.unmakeMove();
	}

	return nodes;
}
