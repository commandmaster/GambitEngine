#pragma once

#include <stdint.h>
#include <immintrin.h>
#include <ammintrin.h>
#include <cmath>
#include <string>
#include <array>
#include <cctype>
#include <vector>

typedef uint64_t Bitboard;
typedef uint64_t Square;

#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))

#define _Compiletime static __forceinline constexpr

// Bitboard Layout
// 00 = a8, 63 = h1
// 
// 00 01 02 03 04 05 06 07
// 08 09 10 11 12 13 14 15
// 16 17 18 19 20 21 22 23
// 24 25 26 27 28 29 30 31
// 32 33 34 35 36 37 38 39
// 40 41 42 43 44 45 46 47
// 48 49 50 51 52 53 54 55
// 56 57 58 59 60 61 62 63



namespace Piece
{
	static constexpr uint8_t NONE   = 0b1111; 
	static constexpr uint8_t WP     = 0b0000;
	static constexpr uint8_t WN     = 0b0010;
	static constexpr uint8_t WB     = 0b0100;
	static constexpr uint8_t WR     = 0b0110;
	static constexpr uint8_t WQ     = 0b1000;
	static constexpr uint8_t WK     = 0b1010;
	static constexpr uint8_t BP     = 0b0001;
	static constexpr uint8_t BN     = 0b0011;
	static constexpr uint8_t BB     = 0b0101;
	static constexpr uint8_t BR     = 0b0111;
	static constexpr uint8_t BQ     = 0b1001;
	static constexpr uint8_t BK     = 0b1011;

	static constexpr uint8_t PIECE_MASK = 0b1110; 
	static constexpr uint8_t COLOR_MASK = 0b0001; 

	inline constexpr uint8_t getColor(uint8_t piece) {
		return piece & COLOR_MASK;
	}

	inline constexpr uint8_t getType(uint8_t piece) {
		return (piece & PIECE_MASK) >> 1; 
	}

	inline constexpr uint8_t make(uint8_t color, uint8_t type) {
		return (type << 1) | color; 
	}
}


struct Move
{
	uint8_t startSquare : 6;
	uint8_t endSquare : 6;
	uint8_t piece : 4;
	uint8_t promotedPiece : 4;
	bool captureFlag : 1;
	bool doublePushFlag : 1;
	bool enpassantFlag : 1;
	bool castlingFlag : 1;
};

struct BoardState
{
	struct History 
	{
		Move move;                  // The move made
		uint8_t capturedPiece;      // Type of captured piece (Piece::NONE if none)
		Square capturedSquare;      // Where the capture occurred (if any)
		uint8_t prevCastlingRights; // Castling rights before the move
		Bitboard prevEnPassant;     // En passant state before the move
		uint16_t prevHalfmoveClock; // Halfmove clock before the move
		uint16_t prevFullmoveNumber;// Fullmove number before the move
		Square rookFrom;            // Rook's original square (castling)
		Square rookTo;              // Rook's new square (castling)
	};	


	Bitboard whitePawns;
	Bitboard blackPawns;
	Bitboard whiteKnights;
	Bitboard blackKnights;
	Bitboard whiteBishops;
	Bitboard blackBishops;
	Bitboard whiteRooks;
	Bitboard blackRooks;
	Bitboard whiteQueens;
	Bitboard blackQueens;
	Bitboard whiteKing;
	Bitboard blackKing;

	bool whiteTurn;
    uint8_t castlingRights;
    Bitboard enPassant;
    uint16_t halfmoveClock;
    uint16_t fullmoveNumber;

	std::vector<History> historyStack;

	void makeMove(const Move& move);
	void unmakeMove();

	void parseFEN(const std::string& str);
	std::string exportToFEN() const;

	
	inline Bitboard all() const
	{
		return whitePawns | blackPawns | whiteKnights | blackKnights | whiteBishops | blackBishops | whiteRooks | blackRooks | (whiteQueens | blackQueens) | whiteKing | blackKing;
	}

	inline Bitboard white() const
	{
		return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
	}

	inline Bitboard black() const
	{
		return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
	}
};