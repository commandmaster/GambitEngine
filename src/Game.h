#pragma once

#include <algorithm>
#include <vector>
#include <array>
#include <immintrin.h>
#include <string>
#include <random>
#include <future>
#include <memory>

#include "Board.h"
#include "MoveGenerator.h"
#include "Renderer.h"

#include "raylib.h"

namespace Search
{
	std::random_device dev;
	std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 3);

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

	inline int sumBonuses(Bitboard bb, const std::array<uint16_t, 64>& table) 
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

	constexpr int getPieceValue(uint8_t piece) 
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

	inline uint8_t getCapturedPieceType(const BoardState& board, const Move& move) {
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

	static std::atomic<bool> Timeout = false; 
    static std::chrono::steady_clock::time_point SearchStart; 
    static int TimeLimit = 5000; 

	inline void startTimer() 
	{
        Timeout = false;
        SearchStart = std::chrono::steady_clock::now();
    }

    inline bool timeElapsed() 
	{
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - SearchStart
        ).count();
        return elapsed >= TimeLimit;
    }

    int quiescence(BoardState& board, int alpha, int beta, MoveGenerator& moveGenerator);

    int evaluate(const BoardState& board) 
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

		// Game phase calculation (0 = endgame, 24 = opening)
		int phase = (__popcnt64(board.whiteQueens + board.blackQueens) * 4) +
					(__popcnt64(board.whiteRooks + board.blackRooks) * 2) +
					(__popcnt64(board.whiteBishops + board.blackBishops + 
							  board.whiteKnights + board.blackKnights) * 1);
		const int totalPhase = 24;
		double phaseFactor = std::clamp(phase / (double)totalPhase, 0.0, 1.0);

		// Positional scores with tapered king safety
		int whitePositional = 0, blackPositional = 0;
		
		whitePositional += sumBonuses(board.whitePawns, Search::pawnBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackPawns), Search::pawnBonus);

		whitePositional += sumBonuses(board.whiteKnights, Search::knightBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackKnights), Search::knightBonus);

		whitePositional += sumBonuses(board.whiteBishops, Search::bishopBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackBishops), Search::bishopBonus);

		whitePositional += sumBonuses(board.whiteRooks, Search::rookBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackRooks), Search::rookBonus);

		whitePositional += sumBonuses(board.whiteQueens, Search::queenBonus);
		blackPositional += sumBonuses(mirrorVertical(board.blackQueens), Search::queenBonus);

		// Tapered king safety
		int whiteKingMiddle = sumBonuses(board.whiteKing, Search::kingBonusMiddle);
		int whiteKingEnd = sumBonuses(board.whiteKing, Search::kingBonusEnd);
		int whiteKingScore = static_cast<int>(whiteKingMiddle * phaseFactor + whiteKingEnd * (1 - phaseFactor));
		whitePositional += whiteKingScore;

		Bitboard blackKingMirrored = mirrorVertical(board.blackKing);
		int blackKingMiddle = sumBonuses(blackKingMirrored, Search::kingBonusMiddle);
		int blackKingEnd = sumBonuses(blackKingMirrored, Search::kingBonusEnd);
		int blackKingScore = static_cast<int>(blackKingMiddle * phaseFactor + blackKingEnd * (1 - phaseFactor));
		blackPositional += blackKingScore;

		


		// Total evaluation
		int materialDifference = whiteMaterial - blackMaterial;
		int positionalDifference = whitePositional - blackPositional;
		int totalScore = materialDifference + positionalDifference;

		return totalScore * (board.whiteTurn ? 1 : -1);
	}

	int negamax(BoardState& board, int depth, int alpha, int beta, MoveGenerator& moveGenerator)
	{
		if (Search::timeElapsed() || Search::Timeout) {
			Search::Timeout = true;
			return 0;
		}

		if (depth == 0)
			return quiescence(board, alpha, beta, moveGenerator);

		MoveArr moves;
		int moveCount = moveGenerator.generateLegalMoves(moves, board);

		if (moveCount == 0)
		{
			MoveGenerator mg;
			Bitboard attackedSquares = mg.calculateAttackedSquares(board, !board.whiteTurn);
			Bitboard kingBB = board.whiteTurn ? board.whiteKing : board.blackKing;
			bool inCheck = (attackedSquares & kingBB) != 0;

			if (inCheck) 
			{
				return -1000000; // Checkmate score
			} 
			else
			{
				return 0; // Stalemate (draw)
			}
		}	

		std::vector<std::pair<int, Move>> scoredMoves;
		for (int i = 0; i < moveCount; ++i) {
			const Move& move = moves[i];
			int score = 0;

			// Capture heuristic (MVV-LVA)
			if (move.captureFlag) {
				uint8_t capturedPieceType = Piece::NONE;
				if (move.enpassantFlag) {
					capturedPieceType = board.whiteTurn ? Piece::BP : Piece::WP; // En passant captures a pawn
				} else {
					// Determine the captured piece from the board
					Square endSq = move.endSquare;
					Bitboard endBB = 1ULL << endSq;
					if (board.whitePawns & endBB) capturedPieceType = Piece::WP;
					else if (board.whiteKnights & endBB) capturedPieceType = Piece::WN;
					else if (board.whiteBishops & endBB) capturedPieceType = Piece::WB;
					else if (board.whiteRooks & endBB) capturedPieceType = Piece::WR;
					else if (board.whiteQueens & endBB) capturedPieceType = Piece::WQ;
					else if (board.whiteKing & endBB) capturedPieceType = Piece::WK;
					else if (board.blackPawns & endBB) capturedPieceType = Piece::BP;
					else if (board.blackKnights & endBB) capturedPieceType = Piece::BN;
					else if (board.blackBishops & endBB) capturedPieceType = Piece::BB;
					else if (board.blackRooks & endBB) capturedPieceType = Piece::BR;
					else if (board.blackQueens & endBB) capturedPieceType = Piece::BQ;
					else if (board.blackKing & endBB) capturedPieceType = Piece::BK;
				}
				int victimValue = getPieceValue(capturedPieceType);
				int aggressorValue = getPieceValue(move.piece);
				score += 10000 + (victimValue - aggressorValue); 
			}

			if (move.promotedPiece != Piece::NONE) {
				int promoValue = getPieceValue(move.promotedPiece);
				score += 5000 + promoValue; 
			}

			scoredMoves.emplace_back(score, move);
		}

		// Sort moves in descending order of score
		std::sort(scoredMoves.begin(), scoredMoves.end(), [](const auto& a, const auto& b) {
			return a.first > b.first;
		});

		int bestScore = -1000000;
		for (const auto& [score, move] : scoredMoves) {
			board.makeMove(move);
			int currentScore = -negamax(board, depth - 1, -beta, -alpha, moveGenerator);
			board.unmakeMove();

			bestScore = std::max(bestScore, currentScore);
			alpha = std::max(alpha, bestScore);
			if (alpha >= beta)
				break; // Beta cutoff
		}
		return bestScore;
	}



	int quiescence(BoardState& board, int alpha, int beta, MoveGenerator& moveGenerator)
    {
		if (Search::timeElapsed() || Search::Timeout) 
		{
			Search::Timeout = true;
			//return 0;
		} 

        int stand_pat = evaluate(board);
        if (stand_pat >= beta)
            return beta;
        if (stand_pat > alpha)
            alpha = stand_pat;

        MoveArr moves;
        int moveCount = moveGenerator.generateLegalMoves(moves, board);

        std::vector<std::pair<int, Move>> scoredMoves;
        for (int i = 0; i < moveCount; ++i) {
            const Move& move = moves[i];
            if (move.captureFlag || move.promotedPiece != Piece::NONE) {
                int score = 0;
                if (move.captureFlag) {
                    uint8_t capturedType = getCapturedPieceType(board, move);
                    int victimValue = getPieceValue(capturedType);
                    int aggressorValue = getPieceValue(move.piece);
                    score += 10000 + (victimValue - aggressorValue);
                }
                if (move.promotedPiece != Piece::NONE) {
                    int promoValue = getPieceValue(move.promotedPiece);
                    score += 5000 + promoValue;
                }
                scoredMoves.emplace_back(score, move);
            }
        }

        std::sort(scoredMoves.begin(), scoredMoves.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        for (const auto& [score, move] : scoredMoves) {
            board.makeMove(move);
            int currentScore = -quiescence(board, -beta, -alpha, moveGenerator);
            board.unmakeMove();

            if (currentScore >= beta)
                return beta;
            if (currentScore > alpha)
                alpha = currentScore;
        }

        return alpha;
    }
    
}




class Game
{
public:
	Game()
		: boardHistory(), renderer(1800, 1300), moveGenerator{}, board()
	{
		boardHistory.reserve(40);

		board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		std::cout << "Perft at depth 5: " << perft(5, moveGenerator, board) << '\n';

		loadSounds();
	}

	void loadSounds()
	{
		InitAudioDevice();
		sounds[0] = LoadSound("../assets/chessMove1.mp3");
		sounds[1] = LoadSound("../assets/chessMove2.mp3");
		sounds[2] = LoadSound("../assets/chessMove3.mp3");
	}

	bool update()
	{
		if (renderer.shouldClose()) return false;
		
		if (!board.whiteTurn)
		{
			Timer timer;
			timer.start();
			aiTurn();
			timer.stop();
			std::cout << "AI turn time: " << timer.elapsedTime<std::chrono::milliseconds>() << '\n';
		}
		takeTurn();


		renderer.update(board, selectedSquare);
		
		std::string whiteScore = std::to_string(whiteMaterial() - blackMaterial());
		std::string blackScore = std::to_string(blackMaterial() - whiteMaterial());

		DrawText(whiteScore.c_str(), 25, GetScreenHeight() - 50, 40, BLACK);
		DrawText(blackScore.c_str(), 25, 25, 40, BLACK);

		renderer.endDrawing();
		return true;
	}

private:
	std::vector<BoardState> boardHistory;

	Renderer renderer;
	MoveGenerator moveGenerator;
	BoardState board;
	int8_t selectedSquare = -1;
	int8_t clickedSquare = -1;
	Sound sounds[3];

	inline int whiteMaterial()
	{
		return __popcnt64(board.whitePawns) * 1 +
			   __popcnt64(board.whiteKnights) * 3 +
			   __popcnt64(board.whiteBishops) * 3 +
			   __popcnt64(board.whiteRooks) * 5 +
			   __popcnt64(board.whiteQueens) * 9;
	}

	inline int blackMaterial()
	{
		return __popcnt64(board.blackPawns) * 1 +
			   __popcnt64(board.blackKnights) * 3 +
			   __popcnt64(board.blackBishops) * 3 +
			   __popcnt64(board.blackRooks) * 5 +
			   __popcnt64(board.blackQueens) * 9;
	}

	void addMoveToHistory(const BoardState& board)
	{
		boardHistory.push_back(board);
	}

	void aiTurn()
	{
		Search::startTimer();
		Search::TimeLimit = 600; 
		Search::Timeout = false;

		Move bestMove;
		int bestScore = -1000000;
		int depth = 1;

		while (!Search::Timeout) {
			MoveArr moves;
			int moveCount = moveGenerator.generateLegalMoves(moves, board);
			if (moveCount == 0) return;

			if (depth > 1) {
				for (int i = 0; i < moveCount; ++i) {
					if (moves[i] == bestMove) {
						std::swap(moves[0], moves[i]);
						break;
					}
				}
			}

			int currentDepthBestScore = -1000000;
			Move currentDepthBestMove = moves[0];

			for (int i = 0; i < moveCount; ++i) {
				if (Search::timeElapsed()) {
					Search::Timeout = true;
					break;
				}

				board.makeMove(moves[i]);
				int score = -Search::negamax(board, depth - 1, -1000000, 1000000, moveGenerator);
				board.unmakeMove();

				if (Search::Timeout) break; 

				if (score > currentDepthBestScore) {
					currentDepthBestScore = score;
					currentDepthBestMove = moves[i];
				}
			}

			if (Search::Timeout) break; 

			if (currentDepthBestScore > bestScore) {
				bestScore = currentDepthBestScore;
				bestMove = currentDepthBestMove;
			}

			depth++;
		}

		std::cout << "AI depth: " << depth << "\n";
		if (bestScore != -1000000) {
			board.makeMove(bestMove);
			addMoveToHistory(board);
			PlaySound(sounds[1]);
			renderer.startAnimation(bestMove.startSquare, bestMove.endSquare, 150);
		}
	}

	void takeTurn()
	{
		MoveArr moves{};
		int moveCount = moveGenerator.generateLegalMoves(moves, board);
		if (moveCount == 0)
		{
			MoveGenerator mg;
			Bitboard attackedSquares = mg.calculateAttackedSquares(board, !board.whiteTurn);
			Bitboard kingBB = board.whiteTurn ? board.whiteKing : board.blackKing;
			bool inCheck = (attackedSquares & kingBB) != 0;

			if (inCheck) 
			{
				std::cout << "Check mate!" << '\n';
			} 
			else
			{
				std::cout << "Stalemate" << '\n';
			}

		}


		Bitboard boardToVisualize = 0;
		for (int i = 0; i < moveCount; ++i)
		{
			Move& move = moves[i];
			if (move.startSquare == selectedSquare || move.startSquare == clickedSquare)
			{
				boardToVisualize |= (1ULL << move.endSquare);
			}
		}
		renderer.visualizeBoard(boardToVisualize);

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedSquare == -1 && clickedSquare == -1)
		{
			selectedSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());
			clickedSquare = selectedSquare;
		}
		else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			int8_t endSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());
			for (int i = 0; i < moveCount; ++i)
			{
				Move& move = moves[i];
				if (move.startSquare == clickedSquare && move.endSquare == endSquare)
				{
					board.makeMove(move);
					addMoveToHistory(board);

					clickedSquare = -1;
					selectedSquare = -1;

					renderer.visualizeBoard(0);
					PlaySound(sounds[1]);
					return;
				}
			}

			clickedSquare = endSquare;
		}
		else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			if (selectedSquare != -1 && board.whiteTurn)
			{
				int8_t endSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());

				for (int i = 0; i < moveCount; ++i)
				{
					Move& move = moves[i];
					if (move.startSquare == selectedSquare && move.endSquare == endSquare)
					{
						board.makeMove(move);
						addMoveToHistory(board);
						clickedSquare = -1;

						PlaySound(sounds[1]);
						renderer.visualizeBoard(0);
						break;
					}
				}
			}

			selectedSquare = -1;
		}
	}
};

