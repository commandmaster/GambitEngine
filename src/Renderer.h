#pragma once

#include "raylib.h"

#include <cmath>
#include <iostream>

#include "Board.h"
#include "Timer.h"

class Renderer
{
public:
	static constexpr Color whiteColor = Color(235,210,183,255), blackColor = Color(161,111,90,255);

	Renderer(int screenWidth, int screenHeight);
	void update(const BoardState& boardState) const;
	bool shouldClose() const;
	void visualizeBoard(const Bitboard& board = 0x0);
	~Renderer();

private:
	int screenWidth, screenHeight;
	int boardSize;
	Texture2D piecesTexture;
	
	Bitboard visualizedBoard = 0x0ULL;

	void drawSquares() const;
	void drawPieces(const BoardState& boardState) const;
	void drawPieceType(uint64_t pieceBitboard, int pieceType, int pieceColor) const;

};
