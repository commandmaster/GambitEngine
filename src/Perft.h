#pragma once

#include "Board.h"
#include "MoveGenerator.h"
#include "Timer.h"

uint64_t oldPerft(int depth, MoveGenerator& moveGen, BoardState& board);


template<bool Turn, int Depth>
static uint64_t _perft(MoveGenerator& moveGen, BoardState& board)
{
	MoveArr moveArr;
	int moveCount, i;
	uint64_t nodes = 0;

	if constexpr (Depth == 0)
	{
		return 1ULL;
	}

	moveCount = moveGen.generateLegalMoves<Turn>(moveArr, board);

	for (i = 0; i < moveCount; ++i)
	{
		board.makeMove(moveArr[i]);
		nodes += _perft<!Turn, Depth - 1>(moveGen, board);
		board.unmakeMove();
	}

	return nodes;
}

template<>
static uint64_t _perft<true, 0>(MoveGenerator& moveGen, BoardState& board)
{
	return 1ULL;
}

template<>
static uint64_t _perft<false, 0>(MoveGenerator& moveGen, BoardState& board)
{
	return 1ULL;
}


__forceinline uint64_t perft(int depth, MoveGenerator& moveGen, BoardState& board)
{
	Timer timer;
	timer.start();
	
	uint64_t nodes = 0;
	switch (board.whiteTurn)
	{
	case true:
		switch (depth)
		{
		case 1: nodes = _perft<true, 1>(moveGen, board); break;
		case 2: nodes = _perft<true, 2>(moveGen, board); break;
		case 3: nodes = _perft<true, 3>(moveGen, board); break;
		case 4: nodes = _perft<true, 4>(moveGen, board); break;
		case 5: nodes = _perft<true, 5>(moveGen, board); break;
		case 6: nodes = _perft<true, 6>(moveGen, board); break;
		case 7: nodes = _perft<true, 7>(moveGen, board); break;
		case 8: nodes = _perft<true, 8>(moveGen, board); break;
		case 9: nodes = _perft<true, 9>(moveGen, board); break;
		case 10: nodes = _perft<true, 10>(moveGen, board); break;
		case 11: nodes = _perft<true, 11>(moveGen, board); break;
		case 12: nodes = _perft<true, 12>(moveGen, board); break;
		case 13: nodes = _perft<true, 13>(moveGen, board); break;
		case 14: nodes = _perft<true, 14>(moveGen, board); break;
		case 15: nodes = _perft<true, 15>(moveGen, board); break;
		case 16: nodes = _perft<true, 16>(moveGen, board); break;
		case 17: nodes = _perft<true, 17>(moveGen, board); break;
		default: nodes = _perft<true, 18>(moveGen, board); break;
		}
		break;

	case false:
		switch (depth)
		{
		case 1: nodes = _perft<false, 1>(moveGen, board); break;
		case 2: nodes = _perft<false, 2>(moveGen, board); break;
		case 3: nodes = _perft<false, 3>(moveGen, board); break;
		case 4: nodes = _perft<false, 4>(moveGen, board); break;
		case 5: nodes = _perft<false, 5>(moveGen, board); break;
		case 6: nodes = _perft<false, 6>(moveGen, board); break;
		case 7: nodes = _perft<false, 7>(moveGen, board); break;
		case 8: nodes = _perft<false, 8>(moveGen, board); break;
		case 9: nodes = _perft<false, 9>(moveGen, board); break;
		case 10: nodes = _perft<false, 10>(moveGen, board); break;
		case 11: nodes = _perft<false, 11>(moveGen, board); break;
		case 12: nodes = _perft<false, 12>(moveGen, board); break;
		case 13: nodes = _perft<false, 13>(moveGen, board); break;
		case 14: nodes = _perft<false, 14>(moveGen, board); break;
		case 15: nodes = _perft<false, 15>(moveGen, board); break;
		case 16: nodes = _perft<false, 16>(moveGen, board); break;
		case 17: nodes = _perft<false, 17>(moveGen, board); break;
		default: nodes = _perft<false, 18>(moveGen, board); break;
		}
		break;
	}

	std::cout << "Perft at depth " << depth << ": " << std::dec << nodes << '\n';
	timer.stop();
	std::cout << "Perft Time: "  << std::dec << timer.elapsedTime<std::chrono::milliseconds>() << "\n";
	std::cout << "NPS: " << std::dec << std::fixed << (nodes / timer.elapsedTime<std::chrono::milliseconds>()) * 1000 << "\n";

	return nodes;
}

