#pragma once

#include <array>
#include <string>
#include <vector>
#include <type_traits>
#include <thread>

#include "Board.h"
#include "MoveGenerator.h"
#include "Renderer.h"
#include "Search.h"
#include "TranspositionTable.h"
#include "Test.h"
#include "Timer.h"

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include "imgui.h"

class GameLayer : public Walnut::Layer
{
public:
	int moveCount;
	MoveArr moves;
	Searcher searcher;
	MoveGenerator moveGen;
	BoardState board;
	std::unique_ptr<Walnut::Image> piecesSpriteSheet;
	std::string winName;

	std::vector<BoardState> gameHistory; // todo use the built in history stack to save memory
	size_t historyPointer = 0;

	Bitboard legalMovesToVisualize = 0;

	int8_t hoveredSq = -1;
	int8_t selectedSq = -1;

	Timer lerpTimer;
	int animLength = 1000; // Default value but will be changed when animStartSq and animEndSq is set
	int8_t animStartSq = -1;
	int8_t animEndSq = -1;

	char buf[128] = "";

	std::vector<std::string> logBuffer;
	bool autoScrollToTop = true;

	struct {
		uint32_t aiMoveLengthLimit = 1000;
		bool isWhiteAi = false;
		bool isBlackAi = false;
	} boardSettings;

	GameLayer(const std::string& windowName = std::string("New Game"))
		: moveCount{ 0 }, moves{}, moveGen{}, board{}, searcher{}
	{
		winName = windowName;
		gameHistory.reserve(50);

		board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		gameHistory.push_back(board);

		searcher.loadOpeningBook("assets/baron30.bin");

		piecesSpriteSheet = std::make_unique<Walnut::Image>("assets/chessPieces.png");
		loadIcons();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin(winName.c_str());
		
		ImVec2 min = ImGui::GetCursorScreenPos();  
		ImVec2 max = ImGui::GetWindowPos();       
		max.x += ImGui::GetWindowContentRegionMax().x;
		max.y += ImGui::GetWindowContentRegionMax().y;

		ImVec2 windowSize;
		windowSize.x = max.x - min.x;
		windowSize.y = max.y - min.y;

		int boardSize = std::min<int>(windowSize.x, windowSize.y);

		hoveredSq = Rendering::MouseToSquare(boardSize);
		Rendering::DrawMoves(legalMovesToVisualize, boardSize);
		Rendering::DrawBoard(boardSize, ImGui::GetWindowDrawList());

		lerpTimer.stop();
		Rendering::DrawPieces(board, boardSize, piecesSpriteSheet, selectedSq, hoveredSq, animStartSq, animEndSq, 0.001 * lerpTimer.elapsedTime<std::chrono::microseconds>() / (double)animLength);
		ImGui::End();
		
		ImGui::Begin("Board Settings");

		float buttonSize = 40.f;

		// To start of history
		bool toStart = ImGui::ImageButton(icons[(size_t)IconIndex::play_skip_backward]->GetDescriptorSet(), ImVec2(buttonSize, buttonSize), ImVec2(0,0), ImVec2(1,1), -1, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 1));
		ImGui::SameLine(); 

		// Backward move
		bool back = ImGui::ImageButton(icons[(size_t)IconIndex::play_flipped]->GetDescriptorSet(), ImVec2(buttonSize, buttonSize), ImVec2(0,0), ImVec2(1,1), -1, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 1));
		ImGui::SameLine(); 

		// Forward move
		bool forward = ImGui::ImageButton(icons[(size_t)IconIndex::play]->GetDescriptorSet(), ImVec2(buttonSize, buttonSize), ImVec2(0,0), ImVec2(1,1), -1, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 1));
		ImGui::SameLine(); 

		// To end of history
		bool toEnd = ImGui::ImageButton(icons[(size_t)IconIndex::play_skip_forward]->GetDescriptorSet(), ImVec2(buttonSize, buttonSize), ImVec2(0,0), ImVec2(1,1), -1, ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 1));

		if (toStart)
		{
			historyPointer = 0;
			board = gameHistory[0];
		}
		if (toEnd)
		{
			historyPointer = gameHistory.size() - 1;
			board = gameHistory.back();
		}
		if (forward)
		{
			advance();
		}
		if (back)
		{
			goBack();
		}

		ImGui::Text("AI Color: ");
		ImGui::SameLine();
		ImGui::Checkbox("White", &boardSettings.isWhiteAi);
		ImGui::SameLine();
		ImGui::Checkbox("Black", &boardSettings.isBlackAi);

		ImGui::InputInt("AI Time Limit (ms)", (int*)&boardSettings.aiMoveLengthLimit);
		
		// Serial commincation:

		ImGui::Text("Serial Communication:");
		
		if (ImGui::InputText("Write", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
			// This block is only entered when Enter is pressed
			printf("Enter was pressed. Input: %s\n", buf);
			// send it here

			logBuffer.insert(logBuffer.begin(), buf); // Insert at top
			buf[0] = '\0'; // reset
			autoScrollToTop = true; // Flag to scroll to top

		}

		ImGui::BeginChild("LogRegion", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& line : logBuffer) {
			ImGui::TextUnformatted(line.c_str());
		}

		if (autoScrollToTop) {
			ImGui::SetScrollY(0.0f);  // Scroll to the top
			autoScrollToTop = false;  // Reset the flag
		}

		ImGui::EndChild();
		
		ImGui::End();

		//ImGui::ShowDemoWindow();
	}

	virtual void OnUpdate(float ts) override
	{
		if (board.whiteTurn && boardSettings.isWhiteAi)
		{
			lerpTimer.stop();
			if (lerpTimer.elapsedTime<std::chrono::microseconds>() * 0.001 < animLength) return;
			aiTurn(true);
		}
		else if (board.whiteTurn)
		{
			playerTurn(true);
		}
		else if (boardSettings.isBlackAi)
		{
			lerpTimer.stop();
			if (lerpTimer.elapsedTime<std::chrono::microseconds>() * 0.001 < animLength) return;
			aiTurn(false);
		}
		else
		{
			playerTurn(false);
		}
	}

	virtual void OnRender() override 
	{
	
	}

private:
	void playMove(Move& move)
	{
		if (historyPointer + 1 < gameHistory.size())
		{
			gameHistory.resize(historyPointer + 1); // Truncate future moves
		}
		board.makeMove(move);
		gameHistory.push_back(board);
		++historyPointer;
	}

	void advance()
	{
		if (historyPointer + 1 < gameHistory.size()) 
		{
			board = gameHistory[++historyPointer];
		}
	}

	void goBack()
	{
		if (historyPointer > 0)
		{
			board = gameHistory[--historyPointer];
		}
	}

	void playerTurn(bool playerColor)
	{
		if (playerColor != board.whiteTurn) return;

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && selectedSq == -1)
		{
			selectedSq = hoveredSq;
			legalMovesToVisualize = 0;

			if (board.whiteTurn)
				moveCount = moveGen.generateLegalMoves<true>(moves, board);
			else
				moveCount = moveGen.generateLegalMoves<false>(moves, board);

			for (int i = 0; i < moveCount; ++i)
			{
				auto& move = moves[i];
				if (move.startSquare == selectedSq)
				{
					legalMovesToVisualize |= (1ULL << move.endSquare);
				}
			}

			if (legalMovesToVisualize == 0) selectedSq = -1;
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{

			if (board.whiteTurn)
				moveCount = moveGen.generateLegalMoves<true>(moves, board);
			else
				moveCount = moveGen.generateLegalMoves<false>(moves, board);

			for (int i = 0; i < moveCount; ++i)
			{
				auto& move = moves[i];
				if (move.startSquare == selectedSq && move.endSquare == hoveredSq)
				{
					playMove(move);
					break;
				}
			}

			selectedSq = hoveredSq;

			legalMovesToVisualize = 0ULL;
			for (int i = 0; i < moveCount; ++i)
			{
				auto& move = moves[i];
				if (move.startSquare == selectedSq)
				{
					legalMovesToVisualize |= (1ULL << move.endSquare);
				}
			}

			if (legalMovesToVisualize == 0) selectedSq = -1;
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && selectedSq != -1)
		{
			if (board.whiteTurn)
				moveCount = moveGen.generateLegalMoves<true>(moves, board);
			else
				moveCount = moveGen.generateLegalMoves<false>(moves, board);

			for (int i = 0; i < moveCount; ++i)
			{
				auto& move = moves[i];
				if (move.startSquare == selectedSq && move.endSquare == hoveredSq)
				{
					legalMovesToVisualize = 0ULL;
					playMove(move);
					selectedSq = -1;
					break;
				}
			}
		}
		
		
	}

	void aiTurn(bool color)
	{
		if (board.whiteTurn != color) return;

		Move bestMove = searcher.findBestMove(board, 100, boardSettings.aiMoveLengthLimit);
		animStartSq = bestMove.startSquare;
		animEndSq = bestMove.endSquare;
		animLength = 175;

		lerpTimer.stop();
		lerpTimer.start();

		playMove(bestMove);
	}

	enum class IconIndex
	{
		play,
		play_flipped,
		play_skip_forward,
		play_skip_backward,
		COUNT // used to create proper sized array
	};

	constexpr static size_t IconCount = static_cast<size_t>(IconIndex::COUNT);
	std::array<std::unique_ptr<Walnut::Image>, IconCount> icons;

	void loadIcons()
	{
		icons[(size_t)IconIndex::play] = std::make_unique<Walnut::Image>("assets/icons/play.png");
		icons[(size_t)IconIndex::play_flipped] = std::make_unique<Walnut::Image>("assets/icons/play-flipped.png");
		icons[(size_t)IconIndex::play_skip_backward] = std::make_unique<Walnut::Image>("assets/icons/play-skip-back.png");
		icons[(size_t)IconIndex::play_skip_forward] = std::make_unique<Walnut::Image>("assets/icons/play-skip-forward.png");
	}
};


//
//
//
//
//class Game
//{
//public:
//	int i = 0;
//	Timer moveTimer;
//	int mc;
//	MoveArr moves;
//	bool wait = false;

//	Searcher searcher;
//
//	Game()
//		: boardHistory(), renderer(1800, 1300), moveGenerator{}, board(), searcher()
//	{
//		boardHistory.reserve(40);
//		
//		board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//		uint64_t key = computePolyglotHash(board);
//
//		searcher.loadOpeningBook("assets/baron30.bin");
//
//		testDepth(7);
//
//		loadSounds();
//	}
//
//	void loadSounds()
//	{
//
//	}
//
//	bool update()
//	{
//		if (renderer.shouldClose()) return false;
//			
//		if (!board.whiteTurn)
//		{
//			Timer timer;
//			timer.start();
//			aiTurn();
//			timer.stop();
//			std::cout << "AI turn time: " << timer.elapsedTime<std::chrono::milliseconds>() << '\n';
//			std::cout << "Zobrist: " << std::hex << computeZobristHash(board) << " " << board.zobristKey << "\n";
//		}
//		takeTurn();
//
//		renderer.update(board, selectedSquare);
//		
//		std::string whiteScore = std::to_string(whiteMaterial() - blackMaterial());
//		std::string blackScore = std::to_string(blackMaterial() - whiteMaterial());
//
//		DrawText(whiteScore.c_str(), 25, GetScreenHeight() - 50, 40, BLACK);
//		DrawText(blackScore.c_str(), 25, 25, 40, BLACK);
//
//		renderer.endDrawing();
//		return true;
//	}
//
//private:
//	std::vector<BoardState> boardHistory;
//
//	MoveGenerator moveGenerator;
//	BoardState board;
//	int8_t selectedSquare = -1;
//	int8_t clickedSquare = -1;
//
//	inline int whiteMaterial() const
//	{
//		return __popcnt64(board.whitePawns) * 1 +
//			   __popcnt64(board.whiteKnights) * 3 +
//			   __popcnt64(board.whiteBishops) * 3 +
//			   __popcnt64(board.whiteRooks) * 5 +
//			   __popcnt64(board.whiteQueens) * 9;
//	}
//
//	inline int blackMaterial() const
//	{
//		return __popcnt64(board.blackPawns) * 1 +
//			   __popcnt64(board.blackKnights) * 3 +
//			   __popcnt64(board.blackBishops) * 3 +
//			   __popcnt64(board.blackRooks) * 5 +
//			   __popcnt64(board.blackQueens) * 9;
//	}
//
//	void addMoveToHistory(const BoardState& board)
//	{
//		boardHistory.push_back(board);
//	}
//
//	void aiTurn()
//	{
//		Move bestMove = searcher.findBestMove(board, 100, 1000);
//
//		board.makeMove(bestMove);
//		addMoveToHistory(board);
//		PlaySound(sounds[1]);
//		renderer.startAnimation(bestMove.startSquare, bestMove.endSquare, 150);
//	}
//
//	void takeTurn()
//	{
//		MoveArr moves{};
//
//		int moveCount;
//		if (board.whiteTurn) moveCount = moveGenerator.generateLegalMoves<true>(moves, board);
//		else moveCount = moveGenerator.generateLegalMoves<false>(moves, board);
//
//		if (moveCount == 0)
//		{
//			MoveGenerator mg;
//
//			Bitboard attackedSquares;
//			if(board.whiteTurn) attackedSquares = mg.calculateAttackedSquares<false>(board);
//			else attackedSquares = mg.calculateAttackedSquares<true>(board);
//
//			Bitboard kingBB = board.whiteTurn ? board.whiteKing : board.blackKing;
//			bool inCheck = (attackedSquares & kingBB) != 0;
//
//			if (inCheck) 
//			{
//				std::cout << "Check mate!" << '\n';
//			} 
//			else
//			{
//				std::cout << "Stalemate" << '\n';
//			}
//
//		}
//
//
//		Bitboard boardToVisualize = 0;
//		for (int i = 0; i < moveCount; ++i)
//		{
//			Move& move = moves[i];
//			if (move.startSquare == selectedSquare || move.startSquare == clickedSquare)
//			{
//				boardToVisualize |= (1ULL << move.endSquare);
//			}
//		}
//
//		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedSquare == -1 && clickedSquare == -1)
//		{
//			selectedSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());
//			clickedSquare = selectedSquare;
//		}
//		else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
//		{
//			int8_t endSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());
//			for (int i = 0; i < moveCount; ++i)
//			{
//				Move& move = moves[i];
//				if (move.startSquare == clickedSquare && move.endSquare == endSquare)
//				{
//					board.makeMove(move);
//					addMoveToHistory(board);
//
//					clickedSquare = -1;
//					selectedSquare = -1;
//
//					renderer.visualizeBoard(0);
//					PlaySound(sounds[1]);
//					return;
//				}
//			}
//
//			clickedSquare = endSquare;
//		}
//		else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
//		{
//			if (selectedSquare != -1 && board.whiteTurn)
//			{
//				int8_t endSquare = renderer.getClickedSquare(GetMouseX(), GetMouseY());
//
//				for (int i = 0; i < moveCount; ++i)
//				{
//					Move& move = moves[i];
//					if (move.startSquare == selectedSquare && move.endSquare == endSquare)
//					{
//						board.makeMove(move);
//						addMoveToHistory(board);
//						clickedSquare = -1;
//
//						PlaySound(sounds[1]);
//						renderer.visualizeBoard(0);
//						break;
//					}
//				}
//			}
//
//			selectedSquare = -1;
//		}
//	}
//};

