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
#include <utility>
#include <thread>
#include <fstream>
#include <iostream>

#include "Board.h"
#include "MoveGenerator.h"
#include "Opening.h"
#include "Evaluation.h"
#include "TranspositionTable.h"

#define SEARCH_LOGS

class Searcher
{
public:
	static constexpr int MAX_IMPLEMENTED_DEPTH = 40;

	Searcher()
		: rng(dev()), dist(0, 3), openingBookEntries{}, timeout{ false }, bestEval{ INT_MIN }, bestMove{}, bestMoveThisIteration{}, bestEvalThisIteration{ INT_MIN }, ttTable(128)
    {
		#ifdef SEARCH_LOGS
		logFile = std::ofstream("search_logs.txt", std::ios::app);
		if (!logFile)
		{
			std::cerr << "Error opening log file!" << std::endl;
		}
		#endif // SEARCH_LOGS
	}

	~Searcher()
	{
		#ifdef SEARCH_LOGS
		logFile.close();
		#endif // SEARCH_LOGS
	}

	void loadOpeningBook(const std::string& filename) 
	{
        try 
		{
            openingBookEntries = loadPolyglotBook(filename);
			std::cout << "Book Loaded Successfully" << "\n";
        }
		catch (const std::exception& e) 
		{
            std::cerr << "Failed to load opening book: " << e.what() << std::endl;
        }
    }

	Move findBestMove(BoardState& board, int maxDepth, int timeLimit)
	{
		#ifdef SEARCH_LOGS

		Timer timer;
		timer.start();
		logFile << "\n ----Search Start---- \n";
		logFile << "Max depth of: " << std::dec << maxDepth << " - Max time allowed: " << timeLimit << "ms \n";

		#endif // SEARCH_LOGS

		Move bookMove = getBookMove(board);
		if (!bookMove.isNull())
		{
			#ifdef SEARCH_LOGS

			timer.stop();
			logFile << '\n';
			logFile << "Search skipped... Move found in opening table.\n";
			logFile << "Best move is " << moveToUCI(bookMove) << "\n";
			logFile << " ----Search End----\n";
			logFile.flush();

			#endif // SEARCH_LOGS

			return bookMove;
		}

		timeout = false;
		std::thread timerThread(&Searcher::beginTimeout, this, timeLimit);
		timerThread.detach();

		int currentSearchDepth = 1;

		maxDepth = std::min<int>(MAX_IMPLEMENTED_DEPTH, maxDepth);

        for (; currentSearchDepth <= maxDepth; ++currentSearchDepth)
        {
            bestMoveThisIteration = Move{};
            bestEvalThisIteration = INT_MIN;

			#ifdef SEARCH_LOGS
			evaluatedNodes = 0; 
			#endif

            startIterativeSearch(board, currentSearchDepth, board.whiteTurn);

            if (!bestMoveThisIteration.isNull())
            {
                bestMove = bestMoveThisIteration;
                bestEval = bestEvalThisIteration;

				#ifdef SEARCH_LOGS
				int timeElapsed = static_cast<int>(timer.elapsedTime<std::chrono::milliseconds>());
				timer.stop();
				logFile << "Iteration depth: " << currentSearchDepth 
						<< " Best move so far: " << moveToUCI(bestMove) 
						<< " with evaluation of " << bestEval 
						<< " for " << (board.whiteTurn ? "white" : "black") 
						<< ". Time Elapsed: " << timeElapsed << "ms"
						<< " Nodes evaluated: " << std::dec << evaluatedNodes << "\n"; 
				#endif
            }

            if (timeout) break;
        }


		#ifdef SEARCH_LOGS

		timer.stop();
		logFile << '\n';
		logFile << "Search fully completed up to depth: " << currentSearchDepth << " Time taken: " << static_cast<int>(timer.elapsedTime<std::chrono::milliseconds>()) << "ms\n";
		logFile << "Best move is " << moveToUCI(bestMove) << " - Eval: " << bestEval << "\n";
		logFile << " ----Search End----\n";
		logFile.flush();

		#endif // SEARCH_LOGS


        return bestMove;
	}

private:
	void beginTimeout(int timeoutMS) 
	{
		auto start = std::chrono::steady_clock::now();
		while (!timeout.load(std::memory_order_relaxed)) {
			auto elapsed = std::chrono::steady_clock::now() - start;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMS) {
				timeout.store(true, std::memory_order_release);
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	template<int Depth>
    void orderMoves(MoveArr& moves, Move& previousBest, int moveCount, const BoardState& board)
	{
		struct MoveScore 
		{
			Move move;
			int score;
			bool operator>(const MoveScore& other) const { return score > other.score; }
		};
		
		Move hashedMove{};
		auto& data = ttTable.retrieve(board.zobristKey);
		hashedMove = data.move;
		
		std::array<MoveScore, 218> moveScores;

		for (int i = 0; i < moveCount; ++i) {
			moveScores[i].move = moves[i];
			int score = 0;

			// Previous best move gets priority
			if (moves[i] == previousBest || moves[i] == hashedMove)
			{
				score = INT_MAX - 10;
			}
			else 
			{
				// Score calculation logic
				if (moves[i].captureFlag) 
				{
					uint8_t victim = Evaluation::getCapturedPieceType(board, moves[i]);
					score += 10000 + (Evaluation::getPieceValue(victim) * 10)
						   - Evaluation::getPieceValue(moves[i].piece);
				}
				
				if (moves[i].promotedPiece != Piece::NONE) 
				{
					score += 5000 + Evaluation::getPieceValue(moves[i].promotedPiece);
				}
			}
			
			moveScores[i].score = score;
		}

		std::sort(moveScores.begin(), moveScores.begin() + moveCount, std::greater<>());

		for (int i = 0; i < moveCount; ++i) 
		{
			moves[i] = moveScores[i].move;
		}
	}

	Move getBookMove(const BoardState& board) 
	{
        if (openingBookEntries.empty()) return Move{};

        uint64_t key = computePolyglotHash(board);

        auto [lower, upper] = lookupEntries(openingBookEntries, key);
        if (lower == upper) return Move{};


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

    inline void startIterativeSearch(BoardState& board, int depth, bool turn)
    {
        MoveGenerator mg{};
        MoveArr moves{};

        int moveCount;
        if (turn)
			moveCount = mg.generateLegalMoves<true>(moves, board);
        else 
			moveCount = mg.generateLegalMoves<false>(moves, board);


		// We can do this as order moves does a null check on best move for us, if it is not null we search it first
		switch (depth)
		{
		case 0: throw std::runtime_error("You cannot start a search with a depth of 0"); break;
		case 1: orderMoves<1>(moves, bestMove, moveCount, board); break;
		case 2: orderMoves<2>(moves, bestMove, moveCount, board); break;
		case 3: orderMoves<3>(moves, bestMove, moveCount, board); break;
		case 4: orderMoves<4>(moves, bestMove, moveCount, board); break;
		case 5: orderMoves<5>(moves, bestMove, moveCount, board); break;
		case 6: orderMoves<6>(moves, bestMove, moveCount, board); break;
		case 7: orderMoves<7>(moves, bestMove, moveCount, board); break;
		case 8: orderMoves<8>(moves, bestMove, moveCount, board); break;
		case 9: orderMoves<9>(moves, bestMove, moveCount, board); break;
		case 10: orderMoves<10>(moves, bestMove, moveCount, board); break;
		case 11: orderMoves<11>(moves, bestMove, moveCount, board); break;
		case 12: orderMoves<12>(moves, bestMove, moveCount, board); break;
		case 13: orderMoves<13>(moves, bestMove, moveCount, board); break;
		case 14: orderMoves<14>(moves, bestMove, moveCount, board); break;
		case 15: orderMoves<15>(moves, bestMove, moveCount, board); break;
		case 16: orderMoves<16>(moves, bestMove, moveCount, board); break;
		case 17: orderMoves<17>(moves, bestMove, moveCount, board); break;
		case 18: orderMoves<18>(moves, bestMove, moveCount, board); break;
		case 19: orderMoves<19>(moves, bestMove, moveCount, board); break;
		case 20: orderMoves<20>(moves, bestMove, moveCount, board); break;
		case 21: orderMoves<21>(moves, bestMove, moveCount, board); break;
		case 22: orderMoves<22>(moves, bestMove, moveCount, board); break;
		case 23: orderMoves<23>(moves, bestMove, moveCount, board); break;
		case 24: orderMoves<24>(moves, bestMove, moveCount, board); break;
		case 25: orderMoves<25>(moves, bestMove, moveCount, board); break;
		case 26: orderMoves<26>(moves, bestMove, moveCount, board); break;
		case 27: orderMoves<27>(moves, bestMove, moveCount, board); break;
		case 28: orderMoves<28>(moves, bestMove, moveCount, board); break;
		case 29: orderMoves<29>(moves, bestMove, moveCount, board); break;
		case 30: orderMoves<30>(moves, bestMove, moveCount, board); break;
		case 31: orderMoves<31>(moves, bestMove, moveCount, board); break;
		case 32: orderMoves<32>(moves, bestMove, moveCount, board); break;
		case 33: orderMoves<33>(moves, bestMove, moveCount, board); break;
		case 34: orderMoves<34>(moves, bestMove, moveCount, board); break;
		case 35: orderMoves<35>(moves, bestMove, moveCount, board); break;
		case 36: orderMoves<36>(moves, bestMove, moveCount, board); break;
		case 37: orderMoves<37>(moves, bestMove, moveCount, board); break;
		case 38: orderMoves<38>(moves, bestMove, moveCount, board); break;
		case 39: orderMoves<39>(moves, bestMove, moveCount, board); break;
		case 40: orderMoves<40>(moves, bestMove, moveCount, board); break;
		default: throw std::runtime_error("Depth exceeds supported limit"); break;
		}

        int alpha = -20000;
		int beta = 20000;

        for (int i = 0; i < moveCount; ++i)
        {
            auto& move = moves[i];
            int score;

            board.makeMove(move);
            
            switch (!turn)
			{
			case true:
				switch (depth - 1)
				{
				case 0:  score = -negamax<true, 0>(board, -beta, -alpha); break;
				case 1:  score = -negamax<true, 1>(board, -beta, -alpha); break;
				case 2:  score = -negamax<true, 2>(board, -beta, -alpha); break;
				case 3:  score = -negamax<true, 3>(board, -beta, -alpha); break;
				case 4:  score = -negamax<true, 4>(board, -beta, -alpha); break;
				case 5:  score = -negamax<true, 5>(board, -beta, -alpha); break;
				case 6:  score = -negamax<true, 6>(board, -beta, -alpha); break;
				case 7:  score = -negamax<true, 7>(board, -beta, -alpha); break;
				case 8:  score = -negamax<true, 8>(board, -beta, -alpha); break;
				case 9:  score = -negamax<true, 9>(board, -beta, -alpha); break;
				case 10: score = -negamax<true, 10>(board, -beta, -alpha); break;
				case 11: score = -negamax<true, 11>(board, -beta, -alpha); break;
				case 12: score = -negamax<true, 12>(board, -beta, -alpha); break;
				case 13: score = -negamax<true, 13>(board, -beta, -alpha); break;
				case 14: score = -negamax<true, 14>(board, -beta, -alpha); break;
				case 15: score = -negamax<true, 15>(board, -beta, -alpha); break;
				case 16: score = -negamax<true, 16>(board, -beta, -alpha); break;
				case 17: score = -negamax<true, 17>(board, -beta, -alpha); break;
				case 18: score = -negamax<true, 18>(board, -beta, -alpha); break;
				case 19: score = -negamax<true, 19>(board, -beta, -alpha); break;
				case 20: score = -negamax<true, 20>(board, -beta, -alpha); break;
				case 21: score = -negamax<true, 21>(board, -beta, -alpha); break;
				case 22: score = -negamax<true, 22>(board, -beta, -alpha); break;
				case 23: score = -negamax<true, 23>(board, -beta, -alpha); break;
				case 24: score = -negamax<true, 24>(board, -beta, -alpha); break;
				case 25: score = -negamax<true, 25>(board, -beta, -alpha); break;
				case 26: score = -negamax<true, 26>(board, -beta, -alpha); break;
				case 27: score = -negamax<true, 27>(board, -beta, -alpha); break;
				case 28: score = -negamax<true, 28>(board, -beta, -alpha); break;
				case 29: score = -negamax<true, 29>(board, -beta, -alpha); break;
				case 30: score = -negamax<true, 30>(board, -beta, -alpha); break;
				case 31: score = -negamax<true, 31>(board, -beta, -alpha); break;
				case 32: score = -negamax<true, 32>(board, -beta, -alpha); break;
				case 33: score = -negamax<true, 33>(board, -beta, -alpha); break;
				case 34: score = -negamax<true, 34>(board, -beta, -alpha); break;
				case 35: score = -negamax<true, 35>(board, -beta, -alpha); break;
				case 36: score = -negamax<true, 36>(board, -beta, -alpha); break;
				case 37: score = -negamax<true, 37>(board, -beta, -alpha); break;
				case 38: score = -negamax<true, 38>(board, -beta, -alpha); break;
				case 39: score = -negamax<true, 39>(board, -beta, -alpha); break;
				default:
                    throw std::runtime_error("Requested depth is not yet implemented!");
					break;
				}
				break;

			case false:
				switch (depth - 1)
				{
				case 0:  score = -negamax<false, 0>(board, -beta, -alpha); break;
				case 1:  score = -negamax<false, 1>(board, -beta, -alpha); break;
				case 2:  score = -negamax<false, 2>(board, -beta, -alpha); break;
				case 3:  score = -negamax<false, 3>(board, -beta, -alpha); break;
				case 4:  score = -negamax<false, 4>(board, -beta, -alpha); break;
				case 5:  score = -negamax<false, 5>(board, -beta, -alpha); break;
				case 6:  score = -negamax<false, 6>(board, -beta, -alpha); break;
				case 7:  score = -negamax<false, 7>(board, -beta, -alpha); break;
				case 8:  score = -negamax<false, 8>(board, -beta, -alpha); break;
				case 9:  score = -negamax<false, 9>(board, -beta, -alpha); break;
				case 10: score = -negamax<false, 10>(board, -beta, -alpha); break;
				case 11: score = -negamax<false, 11>(board, -beta, -alpha); break;
				case 12: score = -negamax<false, 12>(board, -beta, -alpha); break;
				case 13: score = -negamax<false, 13>(board, -beta, -alpha); break;
				case 14: score = -negamax<false, 14>(board, -beta, -alpha); break;
				case 15: score = -negamax<false, 15>(board, -beta, -alpha); break;
				case 16: score = -negamax<false, 16>(board, -beta, -alpha); break;
				case 17: score = -negamax<false, 17>(board, -beta, -alpha); break;
				case 18: score = -negamax<false, 18>(board, -beta, -alpha); break;
				case 19: score = -negamax<false, 19>(board, -beta, -alpha); break;
				case 20: score = -negamax<false, 20>(board, -beta, -alpha); break;
				case 21: score = -negamax<false, 21>(board, -beta, -alpha); break;
				case 22: score = -negamax<false, 22>(board, -beta, -alpha); break;
				case 23: score = -negamax<false, 23>(board, -beta, -alpha); break;
				case 24: score = -negamax<false, 24>(board, -beta, -alpha); break;
				case 25: score = -negamax<false, 25>(board, -beta, -alpha); break;
				case 26: score = -negamax<false, 26>(board, -beta, -alpha); break;
				case 27: score = -negamax<false, 27>(board, -beta, -alpha); break;
				case 28: score = -negamax<false, 28>(board, -beta, -alpha); break;
				case 29: score = -negamax<false, 29>(board, -beta, -alpha); break;
				case 30: score = -negamax<false, 30>(board, -beta, -alpha); break;
				case 31: score = -negamax<false, 31>(board, -beta, -alpha); break;
				case 32: score = -negamax<false, 32>(board, -beta, -alpha); break;
				case 33: score = -negamax<false, 33>(board, -beta, -alpha); break;
				case 34: score = -negamax<false, 34>(board, -beta, -alpha); break;
				case 35: score = -negamax<false, 35>(board, -beta, -alpha); break;
				case 36: score = -negamax<false, 36>(board, -beta, -alpha); break;
				case 37: score = -negamax<false, 37>(board, -beta, -alpha); break;
				case 38: score = -negamax<false, 38>(board, -beta, -alpha); break;
				case 39: score = -negamax<false, 39>(board, -beta, -alpha); break;
				default:
                    throw std::runtime_error("Requested depth is not yet implemented!");
                    break;
				}
				break;
			}

            board.unmakeMove();

			if (timeout) return;

            if (score > bestEvalThisIteration) 
            {
				bestEvalThisIteration = score;
				bestMoveThisIteration = move;
			}

			alpha = std::max(alpha, score);
			if (alpha >= beta) break;
        }
    }

	template<bool Turn, int Depth>
    int negamax(BoardState& board, int alpha, int beta)
    {
		if (timeout) return 0;

		int originalAlpha = alpha;

		if (board.historyStack.size() >= 2)
		{
			if (board.historyStack[board.historyStack.size() - 2].prevZobristKey == board.zobristKey) 
				return -5; // draw by repetition - offset slightly prefer moves that may be more equal but don't lead to a draw
		}

		#ifdef SEARCH_LOGS
		++evaluatedNodes; 
		#endif

		TTEntry::SmpData& data = ttTable.retrieve(board.zobristKey);
		if (data.depth >= Depth) // data.depth will be 0 if null result is found and thus it will never be used as 'Depth' is always >= 1 during the main search
		{
			int ttScore = data.score;

			#ifdef SEARCH_LOGS
				//logFile << "Found TT Entry" << "\n";
			#endif // SEARCH_LOGS

			if (data.flags == TTEntry::EXACT)
			{
				return ttScore;
			}
			else if (data.flags == TTEntry::LOWERBOUND && ttScore >= beta)
			{
				alpha = std::max<int>(alpha, ttScore);
			}
			else if (data.flags == TTEntry::UPPERBOUND && ttScore <= alpha)
			{
				beta = std::min<int>(beta, ttScore);
			}

			if (alpha >= beta) return ttScore;
		}

        MoveGenerator mg;
		MoveArr moves{};
        int moveCount = mg.generateLegalMoves<Turn>(moves, board);

        if (moveCount == 0)
        {
            if (mg.inCheck)
            {
                constexpr int MATESCORE = -19000 - Depth;
                return MATESCORE;
            }
            else
            {
                constexpr int DRAWSCORE = 0;
                return DRAWSCORE;
            }
        }

        orderMoves<Depth>(moves, bestMove, moveCount, board);

        int bestScore = -25000;
		Move bestMoveInCurrentSearch{};

        for (int i = 0; i < moveCount; ++i) 
        {
			Move& move = moves[i];
			board.makeMove(move);
			int score = -negamax<!Turn, Depth - 1>(board, -beta, -alpha);
			board.unmakeMove();

			if (timeout) return 0;

			if (score > bestScore) 
            {
				if (score > alpha) alpha = score;

				bestScore = score;
				bestMoveInCurrentSearch = move;
				
				if (score >= beta) 
				{
					ttTable.store(board.zobristKey, TTEntry::SmpData{ static_cast<int16_t>(score), static_cast<uint8_t>(Depth), TTEntry::LOWERBOUND, move });
					return score; 
				}
			}
		}

		TTEntry::SmpData newEntryData{};
		newEntryData.score = bestScore;

		if (bestScore <= originalAlpha) newEntryData.flags = TTEntry::UPPERBOUND;
		else if (bestScore >= beta) newEntryData.flags = TTEntry::LOWERBOUND;
		else newEntryData.flags = TTEntry::EXACT;

		newEntryData.depth = Depth;
		newEntryData.move = bestMoveInCurrentSearch;

		ttTable.store(board.zobristKey, newEntryData);

        return bestScore;
    }
    
    template<bool Turn>
    int quiescence(BoardState& board, int alpha, int beta) {
		#ifdef SEARCH_LOGS
		++evaluatedNodes; 
		#endif


        int standPat = Evaluation::evaluate<Turn>(board);
        if (standPat >= beta)
            return beta;
        if (standPat > alpha)
            alpha = standPat;

        MoveGenerator mg;
        MoveArr moves;
        int moveCount = mg.generateLegalMoves<Turn>(moves, board);

        // Filter captures and promotions
        MoveArr qMoves;
        int qCount = 0;
        for (int i = 0; i < moveCount; ++i) {
            if (moves[i].captureFlag || moves[i].promotedPiece != Piece::NONE) {
                qMoves[qCount++] = moves[i];
            }
        }

        // Order moves (without previous best)
        Move nullMove;
        orderMoves<0>(qMoves, nullMove, qCount, board);

        for (int i = 0; i < qCount; ++i) {
            Move& move = qMoves[i];
            board.makeMove(move);
            int score = -quiescence<!Turn>(board, -beta, -alpha);
            board.unmakeMove();

            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }

        return alpha;
    }

	template<>
	int negamax<true, 0>(BoardState& board, int alpha, int beta) {
		return quiescence<true>(board, alpha, beta);
	}

	template<>
	int negamax<false, 0>(BoardState& board, int alpha, int beta) {
		return quiescence<false>(board, alpha, beta);
	}

	std::random_device dev;
    std::mt19937 rng;
    std::uniform_int_distribution<std::mt19937::result_type> dist;

    std::vector<TableEntry> openingBookEntries;

	TranspositionTable ttTable;

    std::atomic<bool> timeout;
    
    Move bestMove;
    int bestEval;

    Move bestMoveThisIteration;
	int bestEvalThisIteration;

	#ifdef SEARCH_LOGS
		std::ofstream logFile;
		uint64_t evaluatedNodes = 0;
	#endif // SEARCH_LOGS


};