#include "Perft.h"

uint64_t oldPerft(int depth, MoveGenerator& moveGen, BoardState& board)
{
	MoveArr moveArr;
	int moveCount, i;
	uint64_t nodes = 0;

	if (depth == 0)
		return 1ULL;

	if (board.whiteTurn) moveCount = moveGen.generateLegalMoves<true>(moveArr, board);
	else moveCount = moveGen.generateLegalMoves<false>(moveArr, board);

	for (i = 0; i < moveCount; ++i)
	{
		board.makeMove(moveArr[i]);
		nodes += oldPerft(depth - 1, moveGen, board);
		board.unmakeMove();
	}

	return nodes;
}
