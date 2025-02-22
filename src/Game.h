#pragma once

#include <array>
#include <string>

#include "Board.h"
#include "MoveGenerator.h"
#include "Renderer.h"
#include "Search.h"

#include "raylib.h"






class Game
{
public:
	int i = 0;
	Timer moveTimer;
	int mc;
	MoveArr moves;
	bool wait = false;

	Game()
		: boardHistory(), renderer(1800, 1300), moveGenerator{}, board()
	{
		boardHistory.reserve(40);
		
		board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		uint64_t key = computeZobristKey(board);
		std::cout << std::hex << key << std::endl;
		
		Timer timer;
		timer.start();
		std::cout << "Perft at depth: " << perft(5, moveGenerator, board) << '\n';
		timer.stop();

		std::cout << "Perft Time: " << timer.elapsedTime<std::chrono::milliseconds>() << "\n";


		Search::loadOpeningBook("assets/baron30.bin");
		loadSounds();
	}

	void loadSounds()
	{
		InitAudioDevice();
		sounds[0] = LoadSound("assets/chessMove1.mp3");
		sounds[1] = LoadSound("assets/chessMove2.mp3");
		sounds[2] = LoadSound("assets/chessMove3.mp3");
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
		Move bookMove = Search::getTableMove(board);
		if (bookMove.startSquare != bookMove.endSquare)
		{
			MoveArr moves;
			int moveCount = moveGenerator.generateLegalMoves(moves, board);
			if (moveCount == 0) return;

			/*for (int i = 0; i < moveCount; ++i) {
				if (moves[i].startSquare == bookMove.startSquare && moves[i].endSquare == bookMove.endSquare)
			}*/

			board.makeMove(bookMove);
			std::cout << "Book Move!" << "\n";
			return;
		}

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

