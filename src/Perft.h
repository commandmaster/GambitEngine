#pragma once

#include "Board.h"
#include "MoveGenerator.h"


uint64_t perft(int depth, MoveGenerator& moveGen, BoardState& board);
