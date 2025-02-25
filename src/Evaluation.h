#pragma once

#include <algorithm>
#include <vector>
#include <array>
#include <immintrin.h>
#include <string>
#include <random>
#include <future>
#include <iterator>
#include <memory>
#include <numeric>

#include "Board.h"
#include "MoveGenerator.h"


namespace Evaluation
{
	static constexpr std::array<uint16_t, 64> pawnBonus = {
		 0,  0,  0,  0,  0,  0,  0,  0,
		50, 50, 50, 50, 50, 50, 50, 50,
		10, 10, 20, 30, 30, 20, 10, 10,
		 5,  5, 10, 25, 25, 10,  5,  5,
		 0,  0,  0, 20, 20,  0,  0,  0,
		 5, -5,-10,  0,  0,-10, -5,  5,
		 5, 10, 10,-20,-20, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	static constexpr std::array<uint16_t, 64> knightBonus = {
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50	
	};
	static constexpr std::array<uint16_t, 64> bishopBonus = {
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  5,  0,  0,  0,  0,  5,-10,
		-20,-10,-10,-10,-10,-10,-10,-20	
	};
	static constexpr std::array<uint16_t, 64> rookBonus = {
		0,  0,  0,  0,  0,  0,  0,  0,
		5, 10, 10, 10, 10, 10, 10,  5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		0,  0,  0,  5,  5,  0,  0,  0 
	};
	static constexpr std::array<uint16_t, 64> queenBonus = {
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20		 
	};
	static constexpr std::array<uint16_t, 64> kingBonusMiddle = {
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30, 10,  0,  0, 10, 30, 20
	};
	static constexpr std::array<uint16_t, 64> kingBonusEnd = {
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  0,  0,  0,  0,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50		
	};



	__forceinline static int sumBonuses(Bitboard bb, const std::array<uint16_t, 64>& table)
	{
		const uint16_t* ptr = table.data();
		__m128i sum = _mm_setzero_si128();
		const __m128i bit_mask = _mm_set_epi16(0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01);

		for (int i = 0; i < 64; i += 8) {
			__m128i bonuses = _mm_loadu_si128((__m128i*)(ptr + i));
			uint8_t mask8 = (bb >> i) & 0xFF;
			__m128i mask = _mm_set1_epi16(mask8);

			// Check which bits are set
			__m128i and_result = _mm_and_si128(mask, bit_mask);
			__m128i cmp = _mm_cmpeq_epi16(and_result, bit_mask);

			// Apply mask and accumulate
			__m128i masked = _mm_and_si128(bonuses, cmp);
			sum = _mm_add_epi16(sum, masked);
		}

		// Horizontal sum
		sum = _mm_add_epi32(_mm_cvtepi16_epi32(sum), _mm_cvtepi16_epi32(_mm_srli_si128(sum, 8)));
		sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
		return _mm_extract_epi32(sum, 0) + _mm_extract_epi32(sum, 1);
	}

	__forceinline static constexpr int getPieceValue(uint8_t piece)
	{
		switch (Piece::getType(piece)) {
		case 0: return 100; // Pawn
		case 1: return 320; // Knight
		case 2: return 330; // Bishop
		case 3: return 500; // Rook
		case 4: return 905; // Queen
		case 5: return 20000; // King
		default: return 0; // None or invalid
		}
	}

	__forceinline static uint8_t getCapturedPieceType(const BoardState& board, const Move& move) {
		if (!move.captureFlag) return Piece::NONE;
		if (move.enpassantFlag) {
			return board.whiteTurn ? Piece::BP : Piece::WP; // En passant captures a pawn
		}
		Square endSq = move.endSquare;
		Bitboard endBB = 1ULL << endSq;
		if (board.whitePawns & endBB) return Piece::WP;
		else if (board.whiteKnights & endBB) return Piece::WN;
		else if (board.whiteBishops & endBB) return Piece::WB;
		else if (board.whiteRooks & endBB) return Piece::WR;
		else if (board.whiteQueens & endBB) return Piece::WQ;
		else if (board.whiteKing & endBB) return Piece::WK;
		else if (board.blackPawns & endBB) return Piece::BP;
		else if (board.blackKnights & endBB) return Piece::BN;
		else if (board.blackBishops & endBB) return Piece::BB;
		else if (board.blackRooks & endBB) return Piece::BR;
		else if (board.blackQueens & endBB) return Piece::BQ;
		else if (board.blackKing & endBB) return Piece::BK;
		else return Piece::NONE;
	}

	__forceinline static int evaluate(const BoardState& board)
	{
		// Material calculation
		int whiteMaterial = __popcnt64(board.whitePawns) * getPieceValue(Piece::WP) +
			__popcnt64(board.whiteKnights) * getPieceValue(Piece::WN) +
			__popcnt64(board.whiteBishops) * getPieceValue(Piece::WB) +
			__popcnt64(board.whiteRooks) * getPieceValue(Piece::WR) +
			__popcnt64(board.whiteQueens) * getPieceValue(Piece::WQ);

		int blackMaterial = __popcnt64(board.blackPawns) * getPieceValue(Piece::WP) +
			__popcnt64(board.blackKnights) * getPieceValue(Piece::WN) +
			__popcnt64(board.blackBishops) * getPieceValue(Piece::WB) +
			__popcnt64(board.blackRooks) * getPieceValue(Piece::WR) +
			__popcnt64(board.blackQueens) * getPieceValue(Piece::WQ);

		int phase = (__popcnt64(board.whiteQueens + board.blackQueens) * 4) +
			(__popcnt64(board.whiteRooks + board.blackRooks) * 2) +
			(__popcnt64(board.whiteBishops + board.blackBishops +
				board.whiteKnights + board.blackKnights) * 1);
		const int totalPhase = 24;
		double phaseFactor = std::clamp(phase / (double)totalPhase, 0.0, 1.0);

		int whitePositional = 0, blackPositional = 0;

		whitePositional += sumBonuses(board.whitePawns, Evaluation::pawnBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackPawns), Evaluation::pawnBonus);

		whitePositional += sumBonuses(board.whiteKnights, Evaluation::knightBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackKnights), Evaluation::knightBonus);

		whitePositional += sumBonuses(board.whiteBishops, Evaluation::bishopBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackBishops), Evaluation::bishopBonus);

		whitePositional += sumBonuses(board.whiteRooks, Evaluation::rookBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackRooks), Evaluation::rookBonus);

		whitePositional += sumBonuses(board.whiteQueens, Evaluation::queenBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackQueens), Evaluation::queenBonus);

		// Tapered king safety
		int whiteKingMiddle = sumBonuses(board.whiteKing, Evaluation::kingBonusMiddle);
		int whiteKingEnd = sumBonuses(board.whiteKing, Evaluation::kingBonusEnd);
		int whiteKingScore = static_cast<int>(whiteKingMiddle * phaseFactor + whiteKingEnd * (1 - phaseFactor));
		whitePositional += whiteKingScore;

		Bitboard blackKingMirrored = mirrorVertical(board.blackKing);
		int blackKingMiddle = sumBonuses(blackKingMirrored, Evaluation::kingBonusMiddle);
		int blackKingEnd = sumBonuses(blackKingMirrored, Evaluation::kingBonusEnd);
		int blackKingScore = static_cast<int>(blackKingMiddle * phaseFactor + blackKingEnd * (1 - phaseFactor));
		blackPositional += blackKingScore;



		int materialDifference = whiteMaterial - blackMaterial;
		int positionalDifference = whitePositional - blackPositional;
		int totalScore = materialDifference + positionalDifference;

		return totalScore * (board.whiteTurn ? 1 : -1);
	}

}
