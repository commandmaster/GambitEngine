#pragma once

#include <cmath>
#include <stdint.h>
#include <array>
#include <iostream>

#include "Board.h"
#include "Precomputation.h"


template <typename T>
T shift(T num, int shift) {
    return (shift < 0) ? (num << -shift) : (num >> shift);
}

typedef std::array<Move, 218> MoveArr;


class MoveGenerator
{
public:
	Bitboard cashedCheckMask;
	Bitboard cashedPinHV;
	Bitboard cashedPinD12;
	Bitboard outputForVisualization;

	MoveGenerator()
		: cashedCheckMask{}, cashedPinHV{}, cashedPinD12{}, outputForVisualization{}
	{}

	__forceinline int generateLegalMoves(MoveArr& moves, BoardState& board)
	{
		Bitboard occupied = board.all();
		Bitboard white = board.white();
		Bitboard black = board.black();
		bool whiteTurn = board.whiteTurn;

		uint8_t checkCount = 0;

		Bitboard enemyPawns = whiteTurn ? board.blackPawns : board.whitePawns;
		Bitboard enemyKnights = whiteTurn ? board.blackKnights : board.whiteKnights;
		Bitboard enemyHV = whiteTurn ? (board.blackRooks | board.blackQueens) : (board.whiteRooks | board.whiteQueens);
		Bitboard enemyD12 = whiteTurn ? (board.blackBishops | board.blackQueens) : (board.whiteBishops | board.whiteQueens);
		Bitboard king = whiteTurn ? board.whiteKing : board.blackKing;
		Bitboard friendly = whiteTurn ? white : black;
		Bitboard enemy = whiteTurn ? black : white;
		Square kingSq = SquareOf(king);


		cashedCheckMask = generateCheckMask(whiteTurn, occupied, enemyPawns, enemyKnights, enemyHV, enemyD12, king, checkCount);
		cashedPinHV = generateHVPinMask(occupied, enemyHV, king);
		cashedPinD12 = generateD12PinMask(occupied, enemyD12, king);

		int moveCount = 0;

		if (checkCount >= 2) {
			moveCount = generateKingMoves(moves, moveCount, board, occupied, friendly, enemy);
			return moveCount;
		}

		moveCount = generatePawnMoves(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateKnightMoves(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateBishopMoves(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateRookMoves(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateQueenMoves(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateKingMoves(moves, moveCount, board, occupied, friendly, enemy);

		if (checkCount != 0) return moveCount;
		generateCastlingMoves(moves, moveCount, board, occupied, friendly, enemy); // Castling is separate due to conditions

		return moveCount;
	}

	__forceinline Bitboard calculateAttackedSquares(BoardState& board, bool whiteAttacking) {
		Bitboard attackedSquares = 0;
		Bitboard occupied = board.all() & ~(whiteAttacking ? board.blackKing : board.whiteKing);
		
		Bitboard pieceBoards[6];
		pieceBoards[0] = whiteAttacking ? board.whiteKnights : board.blackKnights;
		pieceBoards[1] = whiteAttacking ? board.whiteBishops : board.blackBishops;
		pieceBoards[2] = whiteAttacking ? board.whiteRooks : board.blackRooks;
		pieceBoards[3] = whiteAttacking ? board.whiteQueens : board.blackQueens;
		pieceBoards[4] = whiteAttacking ? board.whitePawns : board.blackPawns;
		pieceBoards[5] = whiteAttacking ? board.whiteKing : board.blackKing;

		// Knight attacks
		Bitloop(pieceBoards[0]) {
			attackedSquares |= Lookup::lookupKnightMove(SquareOf(pieceBoards[0]));
		}
		// Bishop attacks
		Bitloop(pieceBoards[1]) {
			attackedSquares |= Lookup::lookupBishopMove(occupied, SquareOf(pieceBoards[1]));
		}
		// Rook attacks
		Bitloop(pieceBoards[2]) {
			attackedSquares |= Lookup::lookupRookMove(occupied, SquareOf(pieceBoards[2]));
		}
		// Queen attacks
		Bitloop(pieceBoards[3]) {
			attackedSquares |= Lookup::lookupQueenMove(occupied, SquareOf(pieceBoards[3]));
		}
		// Pawn attacks
		{
			int8_t pawnCaptureDirLeft = whiteAttacking ? 9 : -7;
			int8_t pawnCaptureDirRight = whiteAttacking ? 7 : -9;

			Bitboard notEdgeRight = ~0x8080808080808080ULL;
			Bitboard notEdgeLeft = ~0x0101010101010101ULL;

			attackedSquares |= shift<Bitboard>(pieceBoards[4] & notEdgeLeft, pawnCaptureDirLeft);
			attackedSquares |= shift<Bitboard>(pieceBoards[4] & notEdgeRight, pawnCaptureDirRight);
		}
		// King attacks (King cannot attack king, so no need to check enemy king)
		Bitloop(pieceBoards[5]) {
			attackedSquares |= Lookup::lookupKingMove(SquareOf(pieceBoards[5]));
		}
		return attackedSquares;
	}


private:
	// todo add moves to move arr, increment movecount  l
	__forceinline void handleEP(MoveArr& moves, int& moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& enemyHV)
	{
		Bitboard epRank = board.whiteTurn ? 0xff000000 : 0xff00000000;
		Bitboard notEdgeLeft = board.whiteTurn ? ~0x0101010101010101ULL : ~0x8080808080808080ULL;
		Bitboard notEdgeRight = board.whiteTurn ? ~0x8080808080808080ULL : ~0x0101010101010101ULL;

		const Bitboard& king = board.whiteTurn ? board.whiteKing : board.blackKing;
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

		outputForVisualization = epTarget;

		if ((king & epRank) && (enemyHV & epRank) && (pawns & epRank))
		{
			if (epLeftPawn)
			{
				Bitboard epRemoved = occupied & ~(epTarget | epLeftPawn);
				if (Lookup::lookupRookMove(epRemoved, SquareOf(king)) & epRank & enemyHV)
				{
					board.enPassant = 0;
					return;
				}
			}
			if (epRightPawn)
			{
				Bitboard epRemoved = occupied & ~(epTarget | epRightPawn);
				if (Lookup::lookupRookMove(epRemoved, SquareOf(king)) & epRank & enemyHV)
				{
					board.enPassant = 0;
					return;
				}
			}
		}

		epLeftPawn &= cashedCheckMask;
		epRightPawn &= cashedCheckMask;

		if (epLeftPawn)
		{
			moves[moveCount++] = { static_cast<uint8_t>(SquareOf(epLeftPawn)), static_cast<uint8_t>(SquareOf(board.enPassant)), board.whiteTurn ? Piece::WP : Piece::BP, Piece::NONE, true, false, true, false };
		}

		if (epRightPawn)
		{
			moves[moveCount++] = { static_cast<uint8_t>(SquareOf(epRightPawn)), static_cast<uint8_t>(SquareOf(board.enPassant)), board.whiteTurn ? Piece::WP : Piece::BP, Piece::NONE, true, false, true, false };
		}
	}

	__forceinline Bitboard generateHVPinMask(const Bitboard& occupied, Bitboard enemyHV, const Bitboard& king)
	{
		Bitboard hvPinMask = 0;
		Bitboard kingRookMoves = Lookup::lookupRookMove(occupied, SquareOf(king));

		Bitloop(enemyHV)
		{
			Square enemySq = SquareOf(enemyHV);

			Bitboard overlap = Lookup::lookupRookMove(occupied, enemySq) & kingRookMoves;
			bool flag = (overlap != 0);
			Bitboard conditionMask = (Bitboard)(0 - flag);

			hvPinMask |= (Lookup::pinBetween(enemySq, SquareOf(king)) | 1ULL << enemySq) & conditionMask;
		}

		return hvPinMask;
	}

	__forceinline Bitboard generateD12PinMask(const Bitboard& occupied, Bitboard enemyD12, const Bitboard& king)
	{
		
		Bitboard d12PinMask = 0;
		Bitboard kingBishopMoves = Lookup::lookupBishopMove(occupied, SquareOf(king));

		Bitloop(enemyD12)
		{
			Square enemySq = SquareOf(enemyD12);

			Bitboard overlap = Lookup::lookupBishopMove(occupied, enemySq) & kingBishopMoves;
			bool flag = (overlap != 0);
			Bitboard conditionMask = (Bitboard)(0 - flag);

			d12PinMask |= (Lookup::pinBetween(enemySq, SquareOf(king)) | 1ULL << enemySq) & conditionMask;
		}

		return d12PinMask;
	}

	__forceinline Bitboard generateCheckMask(bool turn, const Bitboard& occupied, Bitboard enemyPawns, Bitboard enemyKnights, Bitboard enemyHV, Bitboard enemyD12, const Bitboard& king, uint8_t& checkCount)
	{
		Bitboard checkMask = 0;

		Bitboard kingMoves;
		Square enemySq;

		kingMoves = Lookup::lookupRookMove(occupied, SquareOf(king));
		Bitboard checkingHV = enemyHV & kingMoves;
		if (checkingHV)
		{
			enemySq = SquareOf(checkingHV);
			checkMask |= (Lookup::pinBetween(SquareOf(king), enemySq) | checkingHV);
			checkCount++;
		}

		kingMoves = Lookup::lookupBishopMove(occupied, SquareOf(king));
		Bitboard checkingD12 = enemyD12 & kingMoves;
		if (checkingD12)
		{
			enemySq = SquareOf(checkingD12);
			checkMask |= (Lookup::pinBetween(SquareOf(king), enemySq) | checkingD12);
			checkCount++;
		}

		kingMoves = Lookup::lookupKnightMove(SquareOf(king));
		checkCount += ((enemyKnights & kingMoves) != 0);
		checkMask |= enemyKnights & kingMoves;

		{
			int8_t pawnCaptureDirLeft = turn ? 9 : -7;
			int8_t pawnCaptureDirRight = turn ? 7 : -9;

			Bitboard notEdgeRight = ~0x8080808080808080ULL;
			Bitboard notEdgeLeft = ~0x0101010101010101ULL;

			kingMoves = shift<Bitboard>(king & notEdgeLeft, pawnCaptureDirLeft) & enemyPawns;
			kingMoves |= (shift<Bitboard>(king & notEdgeRight, pawnCaptureDirRight) & enemyPawns);

			checkCount += (kingMoves != 0);
			checkMask |= kingMoves;
		}


		if (checkMask == 0) checkMask = ULLONG_MAX;

		return checkMask;
	}

	__forceinline int generatePawnMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard pawns = board.whiteTurn ? board.whitePawns : board.blackPawns;
		Bitboard enPassantTarget = board.enPassant;
		uint8_t pawnPiece = board.whiteTurn ? Piece::WP : Piece::BP;
		// These masks mark the destination ranks for promotion and the starting ranks for double pushes.

		
		Bitboard promotionMask = board.whiteTurn ? 0xffULL : 0xff00000000000000ULL;
		Bitboard doublePushMask = board.whiteTurn ? 0xff000000000000ULL : 0xff00ULL;

		int8_t pawnPushDir = board.whiteTurn ? 8 : -8;
		int8_t pawnCaptureDirLeft = board.whiteTurn ? 9 : -7;
		int8_t pawnCaptureDirRight = board.whiteTurn ? 7 : -9;

		Bitboard notEdgeRight = ~0x8080808080808080ULL;
		Bitboard notEdgeLeft = ~0x0101010101010101ULL;

		{
			Bitboard pinnedHv = pawns & cashedPinHV;
			
			{
				Bitboard singlePush = (shift<Bitboard>(pinnedHv, pawnPushDir)) & cashedPinHV & cashedCheckMask & ~occupied;
				Bitboard promotions = singlePush & promotionMask;
				singlePush &= ~promotionMask;

				Bitloop(singlePush)
				{
					int8_t sq = SquareOf(singlePush);
					moves[moveCount++] = { static_cast<uint8_t>(sq + pawnPushDir), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, false, false, false, false };
				}
				Bitloop(promotions)
				{
					int8_t sq = SquareOf(promotions);
					addPromotions(moves, moveCount, sq + pawnPushDir, sq, false, board.whiteTurn);
				}
			}
			
			Bitboard doublePush = shift<Bitboard>((pinnedHv & doublePushMask), (2 * pawnPushDir)) & cashedPinHV & cashedCheckMask & ~(occupied | shift<Bitboard>(occupied, pawnPushDir));
			Bitloop(doublePush)
			{
				uint8_t sq = SquareOf(doublePush);
				moves[moveCount++] = { static_cast<uint8_t>(sq + 2*pawnPushDir), sq, pawnPiece, Piece::NONE, false, true, false, false };
			}
		}
	
		{
			Bitboard pinnedD12 = pawns & cashedPinD12;
			
			Bitboard attackLeft = shift<Bitboard>((pinnedD12 & notEdgeLeft), pawnCaptureDirLeft) & cashedPinD12 & cashedCheckMask & enemy;
			Bitboard attackRight = shift<Bitboard>((pinnedD12 & notEdgeRight), pawnCaptureDirRight) & cashedPinD12 & cashedCheckMask & enemy;

			Bitboard promotionLeft = attackLeft & promotionMask;
			Bitboard promotionRight = attackRight & promotionMask;
			attackLeft &= ~promotionMask;
			attackRight &= ~promotionMask;

			Bitloop(attackLeft)
			{
				int8_t sq = SquareOf(attackLeft);
				moves[moveCount++] = { static_cast<uint8_t>(sq + pawnCaptureDirLeft), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, true, false, false, false };
			}
			Bitloop(attackRight)
			{
				int8_t sq = SquareOf(attackRight);
				moves[moveCount++] = { static_cast<uint8_t>(sq + pawnCaptureDirRight), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, true, false, false, false };
			}
			Bitloop(promotionLeft)
			{
				int8_t sq = SquareOf(promotionLeft);
				addPromotions(moves, moveCount, sq + pawnCaptureDirLeft, static_cast<uint8_t>(sq), true, board.whiteTurn);
			}
			Bitloop(promotionRight)
			{
				int8_t sq = SquareOf(promotionRight);
				addPromotions(moves, moveCount, sq + pawnCaptureDirRight, static_cast<uint8_t>(sq), true, board.whiteTurn);
			}
		}

		{
			Bitboard notPinned = pawns & ~(cashedPinD12 | cashedPinHV);
			Bitboard promotions;
				
			Bitboard singlePush = shift<Bitboard>(notPinned, pawnPushDir) & cashedCheckMask & ~occupied;
			Bitboard doublePush = shift<Bitboard>((notPinned & doublePushMask), (2 * pawnPushDir)) & cashedCheckMask & ~(occupied | shift<Bitboard>(occupied, pawnPushDir));
			Bitboard attackLeft = shift<Bitboard>((notPinned & notEdgeLeft), pawnCaptureDirLeft) & cashedCheckMask & enemy;
			Bitboard attackRight = shift<Bitboard>((notPinned & notEdgeRight), pawnCaptureDirRight) & cashedCheckMask & enemy;

			promotions = singlePush & promotionMask;
			Bitloop(promotions)
			{
				int8_t sq = SquareOf(promotions);
				addPromotions(moves, moveCount, sq + pawnPushDir, sq, false, board.whiteTurn);
			}
			singlePush &= ~promotionMask;
			Bitloop(singlePush)
			{
				int8_t sq = SquareOf(singlePush);
				moves[moveCount++] = { static_cast<uint8_t>(sq + pawnPushDir), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, false, false, false, false };
			}

			Bitloop(doublePush)
			{
				int8_t sq = SquareOf(doublePush);
				moves[moveCount++] = { static_cast<uint8_t>(sq + 2*pawnPushDir), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, false, true, false, false };
			}

			promotions = attackLeft & promotionMask;
			Bitloop(promotions)
			{
				int8_t sq = SquareOf(promotions);
				addPromotions(moves, moveCount, sq + pawnCaptureDirLeft, static_cast<uint8_t>(sq), true, board.whiteTurn);
			}
			promotions = attackRight & promotionMask;
			Bitloop(promotions)
			{
				int8_t sq = SquareOf(promotions);
				addPromotions(moves, moveCount, sq + pawnCaptureDirRight, static_cast<uint8_t>(sq), true, board.whiteTurn);
			}

			attackLeft &= ~promotionMask;
			Bitloop(attackLeft)
			{
				int8_t sq = SquareOf(attackLeft);
				moves[moveCount++] = { static_cast<uint8_t>(sq + pawnCaptureDirLeft), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, true, false, false, false };
			}

			attackRight &= ~promotionMask;
			Bitloop(attackRight)
			{
				int8_t sq = SquareOf(attackRight);
				moves[moveCount++] = { static_cast<uint8_t>(sq + pawnCaptureDirRight), static_cast<uint8_t>(sq), pawnPiece, Piece::NONE, true, false, false, false };
			}
		}

		if (board.enPassant && !(cashedPinD12 & shift<Bitboard>(board.enPassant, pawnPushDir)))
		{
			Bitboard enemyHV = board.whiteTurn ? board.blackRooks | board.blackQueens : board.whiteRooks | board.whiteQueens;
			handleEP(moves, moveCount, board, occupied, enemyHV);
		}
		
		
		return moveCount;
	}
	
	__forceinline int generateKnightMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard knights = board.whiteTurn ? board.whiteKnights : board.blackKnights;
		uint8_t knightPiece = board.whiteTurn ? Piece::WN : Piece::BN;

		knights &= ~(cashedPinHV | cashedPinD12);
		Bitloop(knights)
		{
			Square sq = SquareOf(knights);
			Bitboard targets = Lookup::lookupKnightMove(sq) & ~friendly & cashedCheckMask;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), knightPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures)
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), knightPiece, Piece::NONE, 1, 0, 0, 0 };
			}
		}
		return moveCount;
	}

	__forceinline int generateBishopMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard bishops = board.whiteTurn ? board.whiteBishops : board.blackBishops;
		uint8_t bishopPiece = board.whiteTurn ? Piece::WB : Piece::BB;

		bishops &= ~cashedPinHV;
		Bitboard pinnedBishops = bishops & cashedPinD12;
		Bitboard notPinned = bishops & ~cashedPinD12;

		Bitloop(pinnedBishops)
		{
			Square sq = SquareOf(pinnedBishops);
			Bitboard targets = Lookup::lookupBishopMove(occupied, sq) & ~friendly & cashedCheckMask & cashedPinD12;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), bishopPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures)
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), bishopPiece, Piece::NONE, 1, 0, 0, 0 };
			}
		}

		Bitloop(notPinned)
		{
			Square sq = SquareOf(notPinned);
			Bitboard targets = Lookup::lookupBishopMove(occupied, sq) & ~friendly & cashedCheckMask;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), bishopPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures)
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), bishopPiece, Piece::NONE, 1, 0, 0, 0 };
			}
		}

		return moveCount;
	}

	__forceinline int generateRookMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard rooks = board.whiteTurn ? board.whiteRooks : board.blackRooks;
		uint8_t rookPiece = board.whiteTurn ? Piece::WR : Piece::BR;

		rooks &= ~cashedPinD12;

		Bitboard pinnedRooks = rooks & cashedPinHV;
		Bitboard notPinned = rooks & ~cashedPinHV;

		Bitloop(pinnedRooks)
		{
			Square sq = SquareOf(pinnedRooks);
			Bitboard targets = Lookup::lookupRookMove(occupied, sq) & ~friendly & cashedCheckMask & cashedPinHV;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), rookPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures)
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), rookPiece, Piece::NONE, 1, 0, 0, 0 };
			}

		}

		Bitloop(notPinned)
		{
			Square sq = SquareOf(notPinned);
			Bitboard targets = Lookup::lookupRookMove(occupied, sq) & ~friendly & cashedCheckMask;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), rookPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures)
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), rookPiece, Piece::NONE, 1, 0, 0, 0 };
			}

		}

		return moveCount;
	}

	__forceinline int generateQueenMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard queens = board.whiteTurn ? board.whiteQueens : board.blackQueens;
		uint8_t queenPiece = board.whiteTurn ? Piece::WQ : Piece::BQ;

		Bitloop(queens) 
		{
			Square sq = SquareOf(queens);
			Bitboard pinMask = ULLONG_MAX;
			if ((1ULL << sq) & cashedPinHV) pinMask = cashedPinHV; // Queens can be pinned both ways
			else if ((1ULL << sq) & cashedPinD12) pinMask = cashedPinD12;
			Bitboard targets = Lookup::lookupQueenMove(occupied, sq) & ~friendly & cashedCheckMask & pinMask;
			Bitboard captures = targets & enemy;
			targets &= ~enemy;

			Bitloop(targets) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(targets)), queenPiece, Piece::NONE, 0, 0, 0, 0 };
			}
			Bitloop(captures) 
			{
				moves[moveCount++] = { static_cast<uint8_t>(sq), static_cast<uint8_t>(SquareOf(captures)), queenPiece, Piece::NONE, 1, 0, 0, 0 };
			}
		}
		return moveCount;
	}

	__forceinline int generateKingMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Square kingSq = SquareOf(board.whiteTurn ? board.whiteKing : board.blackKing);
		Bitboard kingMoves = Lookup::lookupKingMove(kingSq) & ~friendly;
		Bitboard attackedSquares = calculateAttackedSquares(board, !board.whiteTurn); // Squares attacked by enemy

		outputForVisualization = attackedSquares;

		kingMoves &= ~attackedSquares;
		uint8_t kingPiece = board.whiteTurn ? Piece::WK : Piece::BK;

		Bitboard captures = kingMoves & enemy;
		kingMoves &= ~enemy;

		Bitloop(kingMoves) {
			moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(SquareOf(kingMoves)), kingPiece, Piece::NONE, 0, 0, 0, 0 };
		}
		Bitloop(captures) {
			moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(SquareOf(captures)), kingPiece, Piece::NONE, 1, 0, 0, 0 };
		}
		return moveCount;
	}

	__forceinline void generateCastlingMoves(MoveArr& moves, int& moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Square kingSq = SquareOf(board.whiteTurn ? board.whiteKing : board.blackKing);
		Bitboard attackedSquares = calculateAttackedSquares(board, !board.whiteTurn);

		if (board.whiteTurn)
		{
			if (board.castlingRights & 1) // White Kingside
			{
				if (!(occupied & (0x6000000000000000)) && !(attackedSquares & (0x7000000000000000))) // Squares f1, g1 empty and e1, f1, g1 not attacked
				{
					moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(62), Piece::WK, Piece::NONE, 0, 0, 0, 1 }; // Move king to g1
				}
			}
			if (board.castlingRights & 2) // White Queenside
			{
				if (!(occupied & (0xe00000000000000)) && !(attackedSquares & (0x1c00000000000000))) // Squares b1, c1, d1 empty and e1, d1, c1 not attacked
				{
					moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(58), Piece::WK, Piece::NONE, 0, 0, 0, 1 }; // Move king to c1
				}
			}
		}
		else
		{
			if (board.castlingRights & 4) // Black Kingside
			{
				if (!(occupied & (0x60)) && !(attackedSquares & (0x70))) // Squares f8, g8 empty and e8, f8, g8 not attacked
				{
					moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(6), Piece::BK, Piece::NONE, 0, 0, 0, 1 }; // Move king to g8
				}
			}
			if (board.castlingRights & 8) // Black Queenside
			{
				if (!(occupied & (0xe)) && !(attackedSquares & (0x1c))) // Squares b8, c8, d8 empty and e8, d8, c8 not attacked
				{
					moves[moveCount++] = { static_cast<uint8_t>(kingSq), static_cast<uint8_t>(2), Piece::BK, Piece::NONE, 0, 0, 0, 1 }; // Move king to c8
				}
			}
		}
	}


	

	// Helper function to add promotion moves
	void addPromotions(MoveArr& moves, int& moveCount, Square from, Square to, bool capture, bool turn) {
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), turn ? Piece::WP : Piece::BP, turn ? Piece::WQ : Piece::BQ, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), turn ? Piece::WP : Piece::BP, turn ? Piece::WR : Piece::BR, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), turn ? Piece::WP : Piece::BP, turn ? Piece::WB : Piece::BB, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), turn ? Piece::WP : Piece::BP, turn ? Piece::WN : Piece::BN, capture, 0, 0, 0 };
	}

};
