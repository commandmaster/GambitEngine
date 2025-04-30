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
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>

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
		: rng(dev()), dist(0, 3), m_openingBookEntries{}, m_timeout{ false }, m_bestEval{ INT_MIN }, m_bestMove{}, m_bestMoveThisIteration{}, m_bestEvalThisIteration{ INT_MIN }, m_ttTable(128)
    {
		#ifdef SEARCH_LOGS
		m_logFile = std::ofstream("search_logs.txt", std::ios::app);
		if (!m_logFile)
		{
			std::cerr << "Error opening log file!" << std::endl;
		}
		#endif // SEARCH_LOGS
	}

	~Searcher()
	{
		#ifdef SEARCH_LOGS
		m_logFile.close();
		#endif // SEARCH_LOGS

		m_mainThreadRunning.store(false, std::memory_order_release);
		if (m_mainSearchThread.joinable())
		{
			m_mainSearchThread.join();
		}
	}

	void loadOpeningBook(const std::string& filename) 
	{
        try 
		{
            m_openingBookEntries = loadPolyglotBook(filename);
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
		m_logFile << "\n ----Search Start---- \n";
		m_logFile << "Max depth of: " << std::dec << maxDepth << " - Max time allowed: " << timeLimit << "ms \n";

		#endif // SEARCH_LOGS

		Move bookMove = getBookMove(board);
		if (!bookMove.isNull())
		{
			#ifdef SEARCH_LOGS

			timer.stop();
			m_logFile << '\n';
			m_logFile << "Search skipped... Move found in opening table.\n";
			m_logFile << "Best move is " << moveToUCI(bookMove) << "\n";
			m_logFile << " ----Search End----\n";
			m_logFile.flush();

			#endif // SEARCH_LOGS

			return bookMove;
		}

		m_timeout = false;
		std::thread timerThread(&Searcher::beginTimeout, this, timeLimit);
		timerThread.detach();

		int currentSearchDepth = 1;

		maxDepth = std::min<int>(MAX_IMPLEMENTED_DEPTH, maxDepth);

        for (; currentSearchDepth <= maxDepth; ++currentSearchDepth)
        {
            m_bestMoveThisIteration = Move{};
            m_bestEvalThisIteration = INT_MIN;

			#ifdef SEARCH_LOGS
			m_evaluatedNodes = 0; 
			#endif

            startIterativeSearch(board, currentSearchDepth, board.whiteTurn);

            if (!m_bestMoveThisIteration.isNull())
            {
                m_bestMove = m_bestMoveThisIteration;
                m_bestEval = m_bestEvalThisIteration;

				#ifdef SEARCH_LOGS
				int timeElapsed = static_cast<int>(timer.elapsedTime<std::chrono::milliseconds>());
				timer.stop();
				m_logFile << "Iteration depth: " << currentSearchDepth 
						<< " Best move so far: " << moveToUCI(m_bestMove) 
						<< " with evaluation of " << m_bestEval 
						<< " for " << (board.whiteTurn ? "white" : "black") 
						<< ". Time Elapsed: " << timeElapsed << "ms"
						<< " Nodes evaluated: " << std::dec << m_evaluatedNodes << "\n"; 
				#endif
            }

            if (m_timeout) break;
        }


		#ifdef SEARCH_LOGS

		timer.stop();
		m_logFile << '\n';
		m_logFile << "Search fully completed up to depth: " << currentSearchDepth << " Time taken: " << static_cast<int>(timer.elapsedTime<std::chrono::milliseconds>()) << "ms\n";
		m_logFile << "Best move is " << moveToUCI(m_bestMove) << " - Eval: " << m_bestEval << "\n";
		m_logFile << " ----Search End----\n";
		m_logFile.flush();

		#endif // SEARCH_LOGS


        return m_bestMove;
	}

	struct AsyncResult
	{
		bool isCompleted;
		Move bestMove;
	};

	std::shared_ptr<AsyncResult> findBestMoveAsync(BoardState& board, int maxDepth, int timeLimit)
	{
		auto result = std::make_shared<AsyncResult>();

		std::function<void()> job = [this, result, &board, maxDepth, timeLimit]()
		{
			result->isCompleted = false;
			result->bestMove = this->findBestMove(board, maxDepth, timeLimit);
			result->isCompleted = true;
		};

		{
			std::lock_guard lock(m_queueMutex);
			m_jobQueue.push(std::move(job));
		}

		m_queueCondition.notify_one();

		return result;
	}

private:
	static void mainSearchThreadFunction(Searcher* searcher)
	{
		while (searcher->m_mainThreadRunning)
		{
			std::function<void()> job;

			{
				std::unique_lock lock(searcher->m_queueMutex);
				searcher->m_queueCondition.wait(lock, [searcher] {return !searcher->m_jobQueue.empty() || !searcher->m_mainThreadRunning; });
				
				job = std::move(searcher->m_jobQueue.front());
				searcher->m_jobQueue.pop();
			}

			job();
		}
	}

	void startMainSearchThread()
	{
		if (m_mainThreadRunning) return;

		m_mainThreadRunning.store(true, std::memory_order_relaxed);
		m_mainSearchThread = std::thread(mainSearchThreadFunction, this);
	}

	void beginTimeout(int timeoutMS) 
	{
		auto start = std::chrono::steady_clock::now();
		while (!m_timeout.load(std::memory_order_relaxed)) 
		{
			auto elapsed = std::chrono::steady_clock::now() - start;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMS) {
				m_timeout.store(true, std::memory_order_release);
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

    void orderMoves(MoveArr& moves, Move& previousBest, int moveCount, const BoardState& board)
	{
		struct MoveScore 
		{
			Move move;
			int score;
			bool operator>(const MoveScore& other) const { return score > other.score; }
		};
		
		Move hashedMove{};
		auto& data = m_ttTable.retrieve(board.zobristKey);
		hashedMove = data.move;
		
		std::array<MoveScore, 218> moveScores;

		for (int i = 0; i < moveCount; ++i) 
		{
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
        if (m_openingBookEntries.empty()) return Move{};

        uint64_t key = computePolyglotHash(board);

        auto [lower, upper] = lookupEntries(m_openingBookEntries, key);
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
		orderMoves(moves, m_bestMove, moveCount, board);
		
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

			if (m_timeout) return;

            if (score > m_bestEvalThisIteration) 
            {
				m_bestEvalThisIteration = score;
				m_bestMoveThisIteration = move;
			}

			alpha = std::max(alpha, score);
			if (alpha >= beta) break;
        }
    }

	template<bool Turn, int Depth>
    int negamax(BoardState& board, int alpha, int beta)
    {
		if (m_timeout) return 0;

		int originalAlpha = alpha;

		if (board.historyStack.size() >= 2)
		{
			if (board.historyStack[board.historyStack.size() - 2].prevZobristKey == board.zobristKey) 
				return -5; // draw by repetition - offset slightly prefer moves that may be more equal but don't lead to a draw
		}

		#ifdef SEARCH_LOGS
		++m_evaluatedNodes; 
		#endif

		TTEntry::SmpData& data = m_ttTable.retrieve(board.zobristKey);
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

		Move nullMove{};
		orderMoves(moves, nullMove, moveCount, board);

        int bestScore = -25000;
		Move bestMoveInCurrentSearch{};

        for (int i = 0; i < moveCount; ++i) 
        {
			Move& move = moves[i];
			board.makeMove(move);
			int score = -negamax<!Turn, Depth - 1>(board, -beta, -alpha);
			board.unmakeMove();

			if (m_timeout) return 0;

			if (score > bestScore) 
            {
				if (score > alpha) alpha = score;

				bestScore = score;
				bestMoveInCurrentSearch = move;
				
				if (score >= beta) 
				{
					m_ttTable.store(board.zobristKey, TTEntry::SmpData{ static_cast<int16_t>(score), static_cast<uint8_t>(Depth), TTEntry::LOWERBOUND, move });
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

		m_ttTable.store(board.zobristKey, newEntryData);

        return bestScore;
    }
    
    template<bool Turn>
    int quiescence(BoardState& board, int alpha, int beta) 
	{
		#ifdef SEARCH_LOGS
		++m_evaluatedNodes; 
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
        for (int i = 0; i < moveCount; ++i) 
		{
            if (moves[i].captureFlag || moves[i].promotedPiece != Piece::NONE) 
			{
                qMoves[qCount++] = moves[i];
            }
        }

        // Order moves (without previous best)
        Move nullMove;
        orderMoves(qMoves, nullMove, qCount, board);

        for (int i = 0; i < qCount; ++i) 
		{
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
	int negamax<true, 0>(BoardState& board, int alpha, int beta) 
	{
		return quiescence<true>(board, alpha, beta);
	}

	template<>
	int negamax<false, 0>(BoardState& board, int alpha, int beta) 
	{
		return quiescence<false>(board, alpha, beta);
	}

	std::random_device dev;
    std::mt19937 rng;
    std::uniform_int_distribution<std::mt19937::result_type> dist;

    std::vector<TableEntry> m_openingBookEntries;

	TranspositionTable m_ttTable;

    std::atomic<bool> m_timeout;

	std::thread m_mainSearchThread;
	std::atomic<bool> m_mainThreadRunning{false};
	std::condition_variable m_queueCondition;
	std::mutex m_queueMutex;
	std::queue<std::function<void()>> m_jobQueue;

	std::vector<std::thread> m_searchThreads;
    
    Move m_bestMove;
    int m_bestEval;

    Move m_bestMoveThisIteration;
	int m_bestEvalThisIteration;

	#ifdef SEARCH_LOGS
		std::ofstream m_logFile;
		uint64_t m_evaluatedNodes = 0;
	#endif // SEARCH_LOGS


};