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
#include "Transposition.h"
#include "Evaluation.h"


namespace Search
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
    static std::uniform_int_distribution<std::mt19937::result_type> dist(0, 3);
	
	

	
    static std::vector<TableEntry> bookEntries;

    static void loadOpeningBook(const std::string& filename) 
	{
        try 
		{
            bookEntries = loadPolyglotBook(filename);
			std::cout << "Book Loaded Successfully" << "\n";
        }
		catch (const std::exception& e) 
		{
            std::cerr << "Failed to load opening book: " << e.what() << std::endl;
        }
    }

    static Move getBookMove(const BoardState& board) 
	{
        if (bookEntries.empty()) return Move{};


        uint64_t key = computeZobristKey(board);

        auto [lower, upper] = lookupEntries(bookEntries, key);
        if (lower == upper) return Move{};

		//std::cout << "found entries" << std::endl;

        std::vector<TableEntry> possibleEntries(lower, upper);
        std::vector<Move> validMoves;
        std::vector<uint16_t> weights;

        MoveGenerator mg;
        MoveArr legalMoves;

		int moveCount;
        if (board.whiteTurn) moveCount = mg.generateLegalMoves<true>(legalMoves, const_cast<BoardState&>(board));
		else moveCount = mg.generateLegalMoves<false>(legalMoves, const_cast<BoardState&>(board));

        for (const auto& entry : possibleEntries) 
		{
            Move bookMove = convertPolyglotMove(entry.move, board.whiteTurn);
			/*std::cout << "Move start sq: " << std::dec << (int)bookMove.startSquare << '\n';
			std::cout << "Move end sq: " << std::dec << (int)bookMove.endSquare << '\n';*/

            for (int i = 0; i < moveCount; ++i) 
			{
                const Move& legalMove = legalMoves[i];
                if (legalMove.startSquare == bookMove.startSquare &&
                    legalMove.endSquare == bookMove.endSquare &&
                    legalMove.promotedPiece == bookMove.promotedPiece) 
				{
					//std::cout << "found valid move" << std::endl;
                    validMoves.push_back(legalMove);
                    weights.push_back(entry.weight);
                    break;
                }
            }
        }

        if (validMoves.empty()) return Move{};

        uint32_t totalWeight = std::accumulate(weights.begin(), weights.end(), 0u);
        if (totalWeight == 0) return Move{};

        std::uniform_int_distribution<uint32_t> dist(0, totalWeight - 1);
        uint32_t r = dist(rng);
        uint32_t cumulative = 0;

        for (size_t i = 0; i < validMoves.size(); ++i)
		{
            cumulative += weights[i];
            if (r < cumulative) return validMoves[i];
        }

        return Move{};
    }


	static void orderMoves(MoveArr& moves, Move& previousBest, int moveCount, const BoardState& board)
    {
        std::array<int, 218> scores{};

        for (int i = 0; i < moveCount; ++i) 
        {
            if (moves[i] == previousBest) 
            {
                scores[i] = INT_MAX;        
				break;
            }
        }

        for (int i = 0; i < moveCount; ++i) 
        {
            if (scores[i] == INT_MAX) continue; 
            
            const Move& move = moves[i];
            scores[i] = 0;

            if (move.captureFlag) 
            {
                uint8_t victim = Evaluation::getCapturedPieceType(board, move);
                scores[i] += 10000 + (Evaluation::getPieceValue(victim) * 10) 
                           - Evaluation::getPieceValue(move.piece);
            }

            if (move.promotedPiece != Piece::NONE)
                scores[i] += 5000 + Evaluation::getPieceValue(move.promotedPiece);

        }

		std::sort(moves.begin(), moves.begin() + moveCount, [&](auto& a, auto& b) {
			auto indexA = &a - moves.data();
			auto indexB = &b - moves.data();

			return scores[indexA] > scores[indexB];	
		});
	}

    template<bool Turn, int Depth>
    static int negamax(BoardState& board, int alpha, int beta)
    {
         
    }
    
	static Move findBestMove(BoardState& board, int maxDepth, int timeLimit)
	{
		
	}
}