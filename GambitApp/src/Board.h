#pragma once

#include <stdint.h>
#include <immintrin.h>
#include <ammintrin.h>
#include <cmath>
#include <string>
#include <array>
#include <cctype>
#include <vector>
#include <string>
#include <iostream>

#include "Zobrist.h"

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


_Compiletime Bitboard mirrorVertical(Bitboard x) 
{
    x = ((x >> 8)  & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
    x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
    x = (x >> 32) | (x << 32);
    return x;
}

_Compiletime uint64_t flipVertical(Bitboard bb) 
{
    bb = ((bb >> 8) & 0x00FFFFFFFFFFFFFFULL) | ((bb & 0x00FFFFFFFFFFFFFFULL) << 8);
    bb = ((bb >> 16) & 0x0000FFFFFFFFFFFFULL) | ((bb & 0x0000FFFFFFFFFFFFULL) << 16);
    bb = ((bb >> 32) & 0x00000000FFFFFFFFULL) | ((bb & 0x00000000FFFFFFFFULL) << 32);
    return bb;
}

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
	__forceinline bool isNull() const
	{
		return startSquare == endSquare;
	}

	bool operator==(const Move& other) const {
		return startSquare == other.startSquare &&
			   endSquare == other.endSquare &&
			   piece == other.piece &&
			   promotedPiece == other.promotedPiece &&
			   captureFlag == other.captureFlag &&
			   doublePushFlag == other.doublePushFlag &&
			   enpassantFlag == other.enpassantFlag &&
			   castlingFlag == other.castlingFlag;
	}

	uint8_t startSquare : 6;
	uint8_t endSquare : 6;
	uint8_t piece : 4;
	uint8_t promotedPiece : 4;
	bool captureFlag : 1;
	bool doublePushFlag : 1;
	bool enpassantFlag : 1;
	bool castlingFlag : 1;
};

static std::string moveToUCI(const Move& move) {
    std::string uci;

    // Convert start square
    auto toFile = [](int square) { return 'a' + (square % 8); };
    auto toRank = [](int square) { return '0' + (8 - (square / 8)); };

    uci += toFile(move.startSquare);
    uci += toRank(move.startSquare);
    uci += toFile(move.endSquare);
    uci += toRank(move.endSquare);

    // Handle promotion
    if (move.promotedPiece != Piece::NONE) {
        switch (Piece::getType(move.promotedPiece)) {
            case 1: uci += 'n'; break;
            case 2: uci += 'b'; break;
            case 3: uci += 'r'; break;
            case 4: uci += 'q'; break;
            default: break; // Invalid promotion (ignored)
        }
    }

    return uci;
}

template <bool turn, bool kw, bool qw, bool kb, bool qb, bool hasEnPassant>
struct BoardStatus 
{
    static constexpr bool whiteToMove = turn;
    static constexpr bool kingsideWhite = kw;
    static constexpr bool queensideWhite = qw;
    static constexpr bool kingsideBlack = kb;
    static constexpr bool queensideBlack = qb;
    static constexpr bool enPassantPossible = hasEnPassant;

    template <bool newTurn, bool newKW, bool newQW, bool newKB, bool newQB, bool newEP>
    using Update = BoardStatus<newTurn, newKW, newQW, newKB, newQB, newEP>;
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
		uint64_t prevZobristKey; // The old zobrist hash for the position
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

	uint64_t zobristKey = 0;

	bool whiteTurn;
    uint8_t castlingRights; // 0b black queenside | black kingside | white queenside | white kingside
    Bitboard enPassant;
    uint16_t halfmoveClock;
    uint16_t fullmoveNumber;

	std::vector<History> historyStack;

	void makeMove(const Move& move);
	void unmakeMove();

	void parseFEN(const std::string& str);
	std::string exportToFEN() const;


	__forceinline Bitboard all() const
	{
		return whitePawns | blackPawns | whiteKnights | blackKnights | whiteBishops | blackBishops | whiteRooks | blackRooks | (whiteQueens | blackQueens) | whiteKing | blackKing;
	}

	__forceinline Bitboard white() const
	{
		return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
	}

	__forceinline Bitboard black() const
	{
		return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
	}
};


// Plain zobrist hashing using our own board format will be faster in the TT as we won't need to do any transformations on the position
__forceinline static uint64_t computeZobristHash(const BoardState& board)
{
	uint64_t key = 0;
	auto process = [&key](Bitboard bb, int pieceIndex)
		{
			Bitloop(bb)
			{
				Square sq = SquareOf(bb); // We don't apply any transformations to the the sq like we do in polyglot as there is no need
				key ^= Random64[pieceIndex * 64 + sq]; // Use the same randoms as polyglot for convenience
			}
		};

	process(board.blackPawns, 0);
    process(board.whitePawns, 1);
    process(board.blackKnights, 2);
    process(board.whiteKnights, 3);
    process(board.blackBishops, 4);
    process(board.whiteBishops, 5);
    process(board.blackRooks, 6);
    process(board.whiteRooks, 7);
    process(board.blackQueens, 8);
    process(board.whiteQueens, 9);
    process(board.blackKing, 10);
    process(board.whiteKing, 11);

	// Castling rights
    if (board.castlingRights & 1) key ^= Random64[768]; // White kingside
    if (board.castlingRights & 2) key ^= Random64[769]; // White queenside
    if (board.castlingRights & 4) key ^= Random64[770]; // Black kingside
    if (board.castlingRights & 8) key ^= Random64[771]; // Black queenside

	Square epSq = SquareOf(board.enPassant);
	uint8_t file = epSq % 8;
	key ^= Random64[772 + file];

	if(!board.whiteTurn) key ^= Random64[780]; // Different side then polyglot

	return key;
}

__forceinline static uint64_t computePolyglotHash(const BoardState& board)
{
    uint64_t key = 0;

    auto process = [&key](Bitboard bb, int pieceIndex)
    {
        Bitloop(bb)
        {
            Square sq = SquareOf(bb);
            int row = 7 - (sq / 8); 
			int file = sq % 8;
            key ^= Random64[pieceIndex * 64 + (row * 8 + file)];
        }

    };

    process(board.blackPawns, 0);
    process(board.whitePawns, 1);
    process(board.blackKnights, 2);
    process(board.whiteKnights, 3);
    process(board.blackBishops, 4);
    process(board.whiteBishops, 5);
    process(board.blackRooks, 6);
    process(board.whiteRooks, 7);
    process(board.blackQueens, 8);
    process(board.whiteQueens, 9);
    process(board.blackKing, 10);
    process(board.whiteKing, 11);

    // Castling rights
    if (board.castlingRights & 1) key ^= Random64[768]; // White kingside
    if (board.castlingRights & 2) key ^= Random64[769]; // White queenside
    if (board.castlingRights & 4) key ^= Random64[770]; // Black kingside
    if (board.castlingRights & 8) key ^= Random64[771]; // Black queenside

    // En passant
    if (board.enPassant)
    {
		Bitboard epRank = board.whiteTurn ? 0xff000000 : 0xff00000000;
		Bitboard notEdgeLeft = board.whiteTurn ? ~0x0101010101010101ULL : ~0x8080808080808080ULL;
        Bitboard notEdgeRight = board.whiteTurn ? ~0x8080808080808080ULL : ~0x0101010101010101ULL;

		const Bitboard& pawns = board.whiteTurn ? board.whitePawns : board.blackPawns;

        Bitboard epTarget = board.enPassant;
		if (board.whiteTurn)
		{
			epTarget <<= 8;
		}
		else
		{
            epTarget >>= 8;
		}

        Bitboard epLeftPawn = pawns & ((epTarget & ~0x0101010101010101ULL) >> 1);
		Bitboard epRightPawn = pawns & ((epTarget & ~0x8080808080808080ULL) << 1);

        if (epLeftPawn | epRightPawn)
        {
			Square epSq = SquareOf(board.enPassant);
			int file = epSq % 8;
			key ^= Random64[772 + file];

        }
    }

    if (board.whiteTurn) key ^= Random64[780];

    return key;
}

