#pragma once

#include <cmath>
#include <stdint.h>
#include <array>
#include <iostream>

#include "Board.h"
#include "Precomputation.h"
#include "Helpers.h"


template <typename T, int shiftValue>
_Compiletime T shift(T num) {
	if constexpr (shiftValue < 0) return num << -shiftValue;
	else return num >> shiftValue;
}

typedef std::array<Move, 218> MoveArr;



class MoveGenerator
{
public:
	Bitboard cashedCheckMask;
	Bitboard cashedPinHV;
	Bitboard cashedPinD12;
	Bitboard outputForVisualization;

	Bitboard whiteAttacked;
	Bitboard blackAttacked;

	bool inCheck = false;

	MoveGenerator()
		: cashedCheckMask{}, cashedPinHV{}, cashedPinD12{}, outputForVisualization{}
	{}

	template<bool Turn>
	__forceinline void initStack(BoardState& board)
	{
		if constexpr (Turn) blackAttacked = calculateAttackedSquares<false>(board);
		else whiteAttacked = calculateAttackedSquares<true>(board);
	}

	template<bool Turn>
	__forceinline int generateLegalMoves(MoveArr& moves, BoardState& board)
	{
		initStack<Turn>(board);
		inCheck = false;

		Bitboard occupied = board.all();
		Bitboard white = board.white();
		Bitboard black = board.black();

		constexpr bool whiteTurn = Turn;
		uint8_t checkCount = 0;

		Bitboard& enemyPawns = Helpers::getEnemyPawns<Turn>(board);
		Bitboard& enemyKnights = Helpers::getEnemyKnights<Turn>(board);
		Bitboard enemyHV = Helpers::getEnemyHV<Turn>(board);
		Bitboard enemyD12 = Helpers::getEnemyD12<Turn>(board);
		Bitboard& king = Helpers::getKing<Turn>(board);
		Bitboard friendly = Helpers::getFriendly<Turn>(board);
		Bitboard enemy = Helpers::getEnemy<Turn>(board);
		Square kingSq = SquareOf(king);


		cashedCheckMask = generateCheckMask<Turn>(whiteTurn, occupied, enemyPawns, enemyKnights, enemyHV, enemyD12, king, checkCount);
		cashedPinHV = generateHVPinMask(occupied, enemyHV, king);
		cashedPinD12 = generateD12PinMask(occupied, enemyD12, king);

		int moveCount = 0;

		if (checkCount >= 2) {
			moveCount = generateKingMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
			inCheck = true;
			return moveCount;
		}

		moveCount = generatePawnMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateKnightMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateBishopMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateRookMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateQueenMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);
		moveCount = generateKingMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy);

		if (checkCount != 0)
		{
			inCheck = true;
			return moveCount;
		}

		generateCastlingMoves<Turn>(moves, moveCount, board, occupied, friendly, enemy); // Castling is separate due to conditions

		return moveCount;
	}
	
	template<bool Turn>
	__forceinline Bitboard calculateAttackedSquares(BoardState& board)
	{
		Bitboard attackedSquares = 0;
		Bitboard occupied = board.all() & ~Helpers::getEnemyKing<Turn>(board);
		
		Bitboard pieceBoards[6];
		pieceBoards[0] = Helpers::getKnights<Turn>(board);
		pieceBoards[1] = Helpers::getBishops<Turn>(board);
		pieceBoards[2] = Helpers::getRooks<Turn>(board);
		pieceBoards[3] = Helpers::getQueens<Turn>(board);
		pieceBoards[4] = Helpers::getPawns<Turn>(board);
		pieceBoards[5] = Helpers::getKing<Turn>(board);

		// Knight attacks
		Bitloop(pieceBoards[0]) 
		{
			attackedSquares |= Lookup::lookupKnightMove(SquareOf(pieceBoards[0]));
		}
		// Bishop attacks
		Bitloop(pieceBoards[1]) 
		{
			attackedSquares |= Lookup::lookupBishopMove(occupied, SquareOf(pieceBoards[1]));
		}
		// Rook attacks
		Bitloop(pieceBoards[2]) 
		{
			attackedSquares |= Lookup::lookupRookMove(occupied, SquareOf(pieceBoards[2]));
		}
		// Queen attacks
		Bitloop(pieceBoards[3]) 
		{
			attackedSquares |= Lookup::lookupQueenMove(occupied, SquareOf(pieceBoards[3]));
		}
		// Pawn attacks
		{
			constexpr int8_t pawnCaptureDirLeft = Helpers::getPawnCaptureDirLeft<Turn>();
			constexpr int8_t pawnCaptureDirRight = Helpers::getPawnCaptureDirRight<Turn>();

			Bitboard notEdgeRight = ~0x8080808080808080ULL;
			Bitboard notEdgeLeft = ~0x0101010101010101ULL;

			attackedSquares |= shift<Bitboard, pawnCaptureDirLeft>(pieceBoards[4] & notEdgeLeft);
			attackedSquares |= shift<Bitboard, pawnCaptureDirRight>(pieceBoards[4] & notEdgeRight);
		}
		// King attacks (King cannot attack king, so no need to check enemy king)
		Bitloop(pieceBoards[5]) {
			attackedSquares |= Lookup::lookupKingMove(SquareOf(pieceBoards[5]));
		}
		return attackedSquares;
	}


private:
	template<bool Turn>
	__forceinline void handleEP(MoveArr& moves, int& moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& enemyHV)
	{
		Bitboard epRank = board.whiteTurn ? 0xff000000 : 0xff00000000;
		Bitboard notEdgeLeft = Helpers::getNotEdgeLeft<Turn>();
		Bitboard notEdgeRight = Helpers::getNotEdgeRight<Turn>();

		const Bitboard& king = Helpers::getKing<Turn>(board);
		const Bitboard& pawns = Helpers::getPawns<Turn>(board);

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

		epLeftPawn &= ~cashedPinHV;
		epRightPawn &= ~cashedPinHV;


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
			Bitboard pinBetween = Lookup::pinBetweenHV(enemySq, SquareOf(king));

			bool flag = (overlap != 0 && pinBetween);
			Bitboard conditionMask = (Bitboard)(0 - flag);

			hvPinMask |= ((pinBetween | (1ULL << enemySq)) & conditionMask);
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
			Bitboard pinBetween = Lookup::pinBetweenD12(enemySq, SquareOf(king));

			bool flag = (overlap != 0 && pinBetween);
			Bitboard conditionMask = (Bitboard)(0 - flag);


			d12PinMask |= ((pinBetween | (1ULL << enemySq)) & conditionMask);
		}

		return d12PinMask;
	}

	template<bool Turn>
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
			constexpr int8_t pawnCaptureDirLeft = Helpers::getPawnCaptureDirLeft<Turn>();
			constexpr int8_t pawnCaptureDirRight = Helpers::getPawnCaptureDirRight<Turn>();

			Bitboard notEdgeRight = ~0x8080808080808080ULL;
			Bitboard notEdgeLeft = ~0x0101010101010101ULL;

			kingMoves = shift<Bitboard, pawnCaptureDirLeft>(king & notEdgeLeft) & enemyPawns;
			kingMoves |= (shift<Bitboard, pawnCaptureDirRight>(king & notEdgeRight) & enemyPawns);

			checkCount += (kingMoves != 0);
			checkMask |= kingMoves;
		}

		if (checkMask == 0) checkMask = ULLONG_MAX;

		return checkMask;
	}
	
	template<bool Turn>
	__forceinline int generatePawnMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard pawns = Helpers::getPawns<Turn>(board);
		Bitboard enPassantTarget = board.enPassant;
		uint8_t pawnPiece;

		if constexpr (Turn) pawnPiece = Piece::WP;
		else pawnPiece = Piece::BP;
		
		constexpr Bitboard promotionMask = Helpers::getPromotionMask<Turn>();
		constexpr Bitboard doublePushMask = Helpers::getDoublePushMask<Turn>();

		constexpr int8_t pawnPushDir = Helpers::getPawnPushDir<Turn>();
		constexpr int8_t pawnCaptureDirLeft = Helpers::getPawnCaptureDirLeft<Turn>();
		constexpr int8_t pawnCaptureDirRight = Helpers::getPawnCaptureDirRight<Turn>();

		constexpr Bitboard notEdgeRight = ~0x8080808080808080ULL;
		constexpr Bitboard notEdgeLeft = ~0x0101010101010101ULL;

		{
			Bitboard pinnedHv = pawns & cashedPinHV;
			
			{
				Bitboard singlePush = (shift<Bitboard, pawnPushDir>(pinnedHv)) & cashedPinHV & cashedCheckMask & ~occupied;
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
					addPromotions<Turn>(moves, moveCount, sq + pawnPushDir, sq, false);
				}
			}
			
			Bitboard doublePush = shift<Bitboard, 2 * pawnPushDir>((pinnedHv & doublePushMask)) & cashedPinHV & cashedCheckMask & ~(occupied | shift<Bitboard, pawnPushDir>(occupied));
			Bitloop(doublePush)
			{
				uint8_t sq = SquareOf(doublePush);
				moves[moveCount++] = { static_cast<uint8_t>(sq + 2*pawnPushDir), sq, pawnPiece, Piece::NONE, false, true, false, false };
			}
		}
	
		{
			Bitboard pinnedD12 = pawns & cashedPinD12;
			
			Bitboard attackLeft = shift<Bitboard, pawnCaptureDirLeft>((pinnedD12 & notEdgeLeft)) & cashedPinD12 & cashedCheckMask & enemy;
			Bitboard attackRight = shift<Bitboard, pawnCaptureDirRight>((pinnedD12 & notEdgeRight)) & cashedPinD12 & cashedCheckMask & enemy;

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
				addPromotions<Turn>(moves, moveCount, sq + pawnCaptureDirLeft, static_cast<uint8_t>(sq), true);
			}
			Bitloop(promotionRight)
			{
				int8_t sq = SquareOf(promotionRight);
				addPromotions<Turn>(moves, moveCount, sq + pawnCaptureDirRight, static_cast<uint8_t>(sq), true);
			}
		}

		{
			Bitboard notPinned = pawns & ~(cashedPinD12 | cashedPinHV);
			Bitboard promotions;
				
			Bitboard singlePush = shift<Bitboard, pawnPushDir>(notPinned) & cashedCheckMask & ~occupied;
			Bitboard doublePush = shift<Bitboard, (2 * pawnPushDir)>((notPinned & doublePushMask)) & cashedCheckMask & ~(occupied | shift<Bitboard, pawnPushDir>(occupied));
			Bitboard attackLeft = shift<Bitboard, pawnCaptureDirLeft>((notPinned & notEdgeLeft)) & cashedCheckMask & enemy;
			Bitboard attackRight = shift<Bitboard, pawnCaptureDirRight>((notPinned & notEdgeRight)) & cashedCheckMask & enemy;

			promotions = singlePush & promotionMask;
			Bitloop(promotions)
			{
				int8_t sq = SquareOf(promotions);
				addPromotions<Turn>(moves, moveCount, sq + pawnPushDir, sq, false);
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
				addPromotions<Turn>(moves, moveCount, sq + pawnCaptureDirLeft, static_cast<uint8_t>(sq), true);
			}
			promotions = attackRight & promotionMask;
			Bitloop(promotions)
			{
				int8_t sq = SquareOf(promotions);
				addPromotions<Turn>(moves, moveCount, sq + pawnCaptureDirRight, static_cast<uint8_t>(sq), true);
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

		if (board.enPassant && !(cashedPinD12 & shift<Bitboard, pawnPushDir>(board.enPassant)))
		{
			Bitboard enemyHV = board.whiteTurn ? board.blackRooks | board.blackQueens : board.whiteRooks | board.whiteQueens;
			handleEP<Turn>(moves, moveCount, board, occupied, enemyHV);
		}
		
		
		return moveCount;
	}
	
	template<bool Turn>
	__forceinline int generateKnightMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard knights = Helpers::getKnights<Turn>(board);
		constexpr uint8_t knightPiece = Turn ? Piece::WN : Piece::BN;

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


	template<bool Turn>
	__forceinline int generateBishopMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard bishops = Helpers::getBishops<Turn>(board);
		constexpr uint8_t bishopPiece = Turn ? Piece::WB : Piece::BB;

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


	template<bool Turn>
	__forceinline int generateRookMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard rooks = Helpers::getRooks<Turn>(board);
		constexpr uint8_t rookPiece = Turn ? Piece::WR : Piece::BR;

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


	template<bool Turn>
	__forceinline int generateQueenMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Bitboard queens = Helpers::getQueens<Turn>(board);
		constexpr uint8_t queenPiece = Turn ? Piece::WQ : Piece::BQ;

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

	template<bool Turn>
	__forceinline int generateKingMoves(MoveArr& moves, int moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Square kingSq = SquareOf(Helpers::getKing<Turn>(board));
		Bitboard kingMoves = Lookup::lookupKingMove(kingSq) & ~friendly;
		Bitboard attackedSquares; 

		if constexpr (Turn) attackedSquares = blackAttacked;
		else attackedSquares = whiteAttacked;

		kingMoves &= ~attackedSquares;
		constexpr uint8_t kingPiece = Turn ? Piece::WK : Piece::BK;

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

	template<bool Turn>
	__forceinline void generateCastlingMoves(MoveArr& moves, int& moveCount, BoardState& board, const Bitboard& occupied, const Bitboard& friendly, const Bitboard& enemy)
	{
		Square kingSq = SquareOf(Helpers::getKing<Turn>(board));
		Bitboard attackedSquares;
		if constexpr (Turn) attackedSquares = blackAttacked;
		else attackedSquares = whiteAttacked;


		if constexpr (Turn)
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

	template<bool Turn>
	void addPromotions(MoveArr& moves, int& moveCount, Square from, Square to, bool capture) {
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), Turn ? Piece::WP : Piece::BP, Turn ? Piece::WQ : Piece::BQ, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), Turn ? Piece::WP : Piece::BP, Turn ? Piece::WR : Piece::BR, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), Turn ? Piece::WP : Piece::BP, Turn ? Piece::WB : Piece::BB, capture, 0, 0, 0 };
		moves[moveCount++] = { static_cast<uint8_t>(from), static_cast<uint8_t>(to), Turn ? Piece::WP : Piece::BP, Turn ? Piece::WN : Piece::BN, capture, 0, 0, 0 };
	}

};
