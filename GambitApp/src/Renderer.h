#pragma once

#include <cmath>
#include <iostream>
#include <memory>

#include "imgui.h"

#include "Board.h"
#include "Timer.h"
#include "MoveGenerator.h"

#include "Walnut/Image.h"

namespace Rendering
{
	static ImU32 whiteColor = IM_COL32(235,210,183,255);
	static ImU32 blackColor = IM_COL32(161,111,90,255);
	
	void DrawBoard(int boardSize, ImDrawList* windowDrawList)
	{
		int squareSize = boardSize / 8;
		for (int i = 0; i < 8; ++i) 
		{
			for (int j = 0; j < 8; ++j)
			{
				ImVec2 min(i * squareSize + ImGui::GetCursorScreenPos().x + (ImGui::GetWindowSize().x - boardSize) / 2, j * squareSize + ImGui::GetCursorScreenPos().y);
				ImVec2 max;

				max.x = min.x + squareSize;
				max.y = min.y + squareSize;

				bool color = (i + (j&1)) & 1;
				if (!color)
					windowDrawList->AddRectFilled(min, max, whiteColor);
				else
					windowDrawList->AddRectFilled(min, max, blackColor);
			}
		}
	}

	void DrawPieces(BoardState& board, int boardSize, std::unique_ptr<Walnut::Image>& piecesSpriteSheet, int selectedSq, int hoveredSq)
	{
		int squareSize = boardSize / 8;

		auto drawBB = [&](Bitboard board, int rowIndex, int columnIndex)
			{
				Bitloop(board)
				{
					Square sq = SquareOf(board);

					ImVec2 min;
					ImVec2 max;
					if (sq == selectedSq && (ImGui::IsMouseDown(ImGuiMouseButton_Left) || (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && selectedSq != hoveredSq)))
					{
						min = ImVec2(ImGui::GetMousePos());
						max.x = min.x + squareSize;
						max.y = min.y + squareSize;

						min.x -= squareSize / 2;
						min.y -= squareSize / 2;

						max.x -= squareSize / 2;
						max.y -= squareSize / 2;
					}
					else
					{
						int x = sq % 8;
						int y = sq / 8;

						min = ImVec2(x * squareSize + ImGui::GetCursorScreenPos().x + (ImGui::GetWindowSize().x - boardSize) / 2, y * squareSize + ImGui::GetCursorScreenPos().y);

						max.x = min.x + squareSize;
						max.y = min.y + squareSize;
					}


					ImGui::GetWindowDrawList()->AddImage(piecesSpriteSheet->GetDescriptorSet(), min, max, ImVec2((float)rowIndex / 6.f, (float)columnIndex / 2), ImVec2(((float)rowIndex + 1.f) / 6.f, ((float)columnIndex + 1.f) / 2.f));
				}
			};

		drawBB(board.whitePawns, 5, 0);
		drawBB(board.blackPawns, 5, 1);
		drawBB(board.whiteKnights, 3, 0);
		drawBB(board.blackKnights, 3, 1);
		drawBB(board.whiteBishops, 2, 0);
		drawBB(board.blackBishops, 2, 1);
		drawBB(board.whiteRooks, 4, 0);
		drawBB(board.blackRooks, 4, 1);
		drawBB(board.whiteQueens, 1, 0);
		drawBB(board.blackQueens, 1, 1);
		drawBB(board.whiteKing, 0, 0);
		drawBB(board.blackKing, 0, 1);
	}

	int8_t MouseToSquare(int boardSize)
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		ImVec2 topLeft;
		topLeft.x = ImGui::GetCursorScreenPos().x + (ImGui::GetWindowSize().x - boardSize) / 2;
		topLeft.y = ImGui::GetCursorScreenPos().y;

		ImVec2 bottomRight;
		bottomRight.x = topLeft.x + boardSize;
		bottomRight.y = topLeft.y + boardSize;

		if (mousePos.x < topLeft.x || mousePos.y < topLeft.y || mousePos.x > bottomRight.x || mousePos.y > bottomRight.y) return -1;

		int x = (static_cast<float>(mousePos.x - topLeft.x) / static_cast<float>(boardSize)) * 8;
		int y = (static_cast<float>(mousePos.y - topLeft.y) / static_cast<float>(boardSize)) * 8;
		

		return y * 8 + x;
	}

	void DrawMoves(Bitboard bb, int boardSize)
	{
		int squareSize = boardSize / 8;
		Bitloop(bb)
		{
			Square sq = SquareOf(bb);

			int x = sq % 8;
			int y = sq / 8;

			ImVec2 min(x * squareSize + ImGui::GetCursorScreenPos().x + (ImGui::GetWindowSize().x - boardSize) / 2, y * squareSize + ImGui::GetCursorScreenPos().y);
			ImVec2 pos;

			pos.x = min.x + squareSize / 2;
			pos.y = min.y + squareSize / 2;

			ImGui::GetForegroundDrawList()->AddCircleFilled(pos, (float)squareSize / 6, IM_COL32(150, 150, 150, 110));
		}
	}


};
