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
	void update(const BoardState& boardState, int8_t selectedSquare);
	void endDrawing() const;
	void drawSinglePiece(uint8_t piece, int x, int y);
	bool shouldClose() const;
	void visualizeBoard(const Bitboard& board = 0x0);
	void startAnimation(int8_t startSq, int8_t endSq, int animLength);
	int8_t getClickedSquare(int x, int y) const;
	~Renderer();

private:
	int screenWidth, screenHeight;
	int boardSize;
	Texture2D piecesTexture;

	int8_t startSquare = -1, endSquare = -1;
	Timer lerpTimer;
	int animLength;
	
	Bitboard visualizedBoard = 0x0ULL;

	void drawSquares() const;
	void drawPieces(const BoardState& boardState, int8_t selectedSquare);
	void drawPieceType(uint64_t pieceBitboard, int pieceType, int pieceColor, int8_t selectedSquare);
};
