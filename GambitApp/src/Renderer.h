#pragma once

#include <cmath>
#include <iostream>
#include <memory>

#include "imgui.h"

#include "Board.h"
#include "Timer.h"
#include "MoveGenerator.h"

#include "Walnut/Image.h"

namespace Rendering
{
	static ImU32 whiteColor = IM_COL32(235,210,183,255);
	static ImU32 blackColor = IM_COL32(161,111,90,255);
	
	void DrawBoard(int boardSize, ImDrawList* windowDrawList);

	void DrawPieces(BoardState& board, int boardSize, std::unique_ptr<Walnut::Image>& piecesSpriteSheet, int selectedSq, int hoveredSq, int animStartSquare = -1, int animEndSquare = -1, float animPercentage = 1);

	int8_t MouseToSquare(int boardSize);

	void DrawMoves(Bitboard bb, int boardSize);
};
