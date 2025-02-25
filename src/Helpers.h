#pragma once

#include "Board.h"

namespace Helpers 
{
    template <bool Turn>
    _Compiletime int8_t getPawnPushDir() 
    {
        if constexpr (Turn) return 8;  
        else return -8;               
    }

    template <bool Turn>
    _Compiletime std::pair<int8_t, int8_t> getPawnCaptureDirs() 
    {
        if constexpr (Turn) return {9, 7};  
        else return {-7, -9};              
    }

	template <bool Turn>
    _Compiletime int8_t getPawnCaptureDirLeft() 
    {
        if constexpr (Turn) return 9;  
        else return -7;              
    }

	template <bool Turn>
    _Compiletime int8_t getPawnCaptureDirRight() 
    {
        if constexpr (Turn) return 7;  
        else return -9;              
    }

    template <bool Turn>
    _Compiletime Bitboard getPromotionMask() 
    {
        if constexpr (Turn) return 0xffULL;              
        else return 0xff00000000000000ULL;              
    }

    template <bool Turn>
    _Compiletime Bitboard getDoublePushMask() 
    {
        if constexpr (Turn) return 0xff000000000000ULL;  
        else return 0xff00ULL;                          
    }

    template <bool Turn>
    _Compiletime Bitboard getNotEdgeLeft() 
    {
        if constexpr (Turn) return ~0x0101010101010101ULL;
        else return ~0x8080808080808080ULL;
    }

    template <bool Turn>
    _Compiletime Bitboard getNotEdgeRight() 
    {
        if constexpr (Turn) return ~0x8080808080808080ULL;
        else return ~0x0101010101010101ULL;
    }

	template <bool Turn>
	_Compiletime Bitboard& getPawns(BoardState& board) 
	{
		if constexpr (Turn) return board.whitePawns;
		else return board.blackPawns;
	}

	template <bool Turn>
	_Compiletime Bitboard& getKnights(BoardState& board) 
	{
		if constexpr (Turn) return board.whiteKnights;
		else return board.blackKnights;
	}

	template <bool Turn>
	_Compiletime Bitboard& getBishops(BoardState& board) 
	{
		if constexpr (Turn) return board.whiteBishops;
		else return board.blackBishops;
	}

	template <bool Turn>
	_Compiletime Bitboard& getRooks(BoardState& board) 
	{
		if constexpr (Turn) return board.whiteRooks;
		else return board.blackRooks;
	}

	template <bool Turn>
	_Compiletime Bitboard& getQueens(BoardState& board) 
	{
		if constexpr (Turn) return board.whiteQueens;
		else return board.blackQueens;
	}

	template <bool Turn>
	_Compiletime Bitboard& getKing(BoardState& board) 
	{
		if constexpr (Turn) return board.whiteKing;
		else return board.blackKing;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyPawns(BoardState& board) 
	{
		if constexpr (Turn) return board.blackPawns;
		else return board.whitePawns;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyKnights(BoardState& board) 
	{
		if constexpr (Turn) return board.blackKnights;
		else return board.whiteKnights;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyBishops(BoardState& board) 
	{
		if constexpr (Turn) return board.blackBishops;
		else return board.whiteBishops;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyRooks(BoardState& board) 
	{
		if constexpr (Turn) return board.blackRooks;
		else return board.whiteRooks;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyQueens(BoardState& board) 
	{
		if constexpr (Turn) return board.blackQueens;
		else return board.whiteQueens;
	}

	template <bool Turn>
	_Compiletime Bitboard& getEnemyKing(BoardState& board) 
	{
		if constexpr (Turn) return board.blackKing;
		else return board.whiteKing;
	}

    template <bool Turn>
    _Compiletime Bitboard getEnemyHV(BoardState& board) 
    {
        if constexpr (Turn) return board.blackRooks | board.blackQueens;
        else return board.whiteRooks | board.whiteQueens;
    }

	template <bool Turn>
	_Compiletime Bitboard getEnemyD12(BoardState& board) 
	{
		return (getEnemyBishops<Turn>(board) | getEnemyQueens<Turn>(board));
	}

    template <bool Turn>
    _Compiletime Bitboard getFriendly(const BoardState& board) 
    {
        if constexpr (Turn) return board.white();
        else return board.black();
    }
	
	template <bool Turn>
    _Compiletime Bitboard getEnemy(const BoardState& board) 
    {
        if constexpr (Turn) return board.black();
        else return board.white();
    }

    static __forceinline Bitboard getOccupied(const BoardState& board) 
    {
        return board.all();
    }

	template <bool Turn>
    _Compiletime Bitboard getEnemyOrEmpty(const BoardState& board) 
    {
        if constexpr (Turn) return ~board.white();
        else return ~board.black();
    }
    

    template <bool Turn>
    _Compiletime uint8_t makePromotionPiece(int promoType) 
    {
        constexpr uint8_t color = Turn ? 0 : 1;
        return (promoType << 1) | color;
    }
}