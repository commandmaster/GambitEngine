#include "Renderer.h"

Renderer::Renderer(int screenWidth, int screenHeight)
	: screenWidth{screenWidth}, screenHeight{screenHeight}
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(screenWidth, screenHeight, "Chess Engine V4");

	boardSize = std::min<int>(screenWidth, screenHeight);
	piecesTexture = LoadTexture("../assets/chessPieces.png");
	
	SetTargetFPS(60);
}

void Renderer::update(const BoardState& boardState) const
{
	BeginDrawing();
	ClearBackground(RAYWHITE);
	
	drawSquares();
    drawPieces(boardState);

	EndDrawing();
}

bool Renderer::shouldClose() const
{
	return WindowShouldClose();
}

void Renderer::visualizeBoard(const Bitboard& board)
{
    visualizedBoard = board;
}

Renderer::~Renderer()
{
	UnloadTexture(piecesTexture);
	CloseWindow();
}

void Renderer::drawSquares() const
{
	int xOffset = screenWidth / 2 - boardSize / 2;
	int yOffset = screenHeight / 2 - boardSize / 2;
	int squareSize = boardSize / 8;
	
	bool toggle = true;
	for (int i = 0; i < 64; ++i)
	{
		int x, y;
		x = (i % 8) * squareSize + xOffset;
		y = (i / 8) * squareSize + yOffset;
        
        Color color = (toggle) ? Renderer::whiteColor : Renderer::blackColor;

		DrawRectangle(x, y, squareSize, squareSize, color);

        if (visualizedBoard & (1ULL << i))
        {
            DrawRectangle(x, y, squareSize, squareSize, Color(220, 10, 10, 140));
        }

		toggle = !toggle;
		toggle = (i % 8 == 7) ? !toggle : toggle;
	}
}

void Renderer::drawPieces(const BoardState& boardState) const
 {
 	constexpr int C_WHITE = 0;
 	constexpr int C_BLACK = 1;

 	constexpr int P_KING = 0;
 	constexpr int P_QUEEN = 1;
 	constexpr int P_BISHOP = 2;
 	constexpr int P_KNIGHT = 3;
 	constexpr int P_ROOK = 4;
 	constexpr int P_PAWN = 5;


 	drawPieceType(boardState.whitePawns,   P_PAWN,   C_WHITE); 
 	drawPieceType(boardState.whiteRooks,   P_ROOK,   C_WHITE); 
 	drawPieceType(boardState.whiteKnights,  P_KNIGHT,  C_WHITE); 
    drawPieceType(boardState.whiteBishops,  P_BISHOP,  C_WHITE); 
 	drawPieceType(boardState.whiteQueens,  P_QUEEN,  C_WHITE); 
    drawPieceType(boardState.whiteKing,    P_KING,   C_WHITE); 

 	drawPieceType(boardState.blackPawns,   P_PAWN,   C_BLACK); 
 	drawPieceType(boardState.blackRooks,   P_ROOK,   C_BLACK); 
 	drawPieceType(boardState.blackKnights,  P_KNIGHT,  C_BLACK); 
 	drawPieceType(boardState.blackBishops,  P_BISHOP,  C_BLACK); 
 	drawPieceType(boardState.blackQueens,  P_QUEEN,  C_BLACK); 
 	drawPieceType(boardState.blackKing,    P_KING,   C_BLACK); 
 }

void Renderer::drawPieceType(uint64_t pieceBitboard, int pieceType, int pieceColor) const 
 {
 	int xOffset = screenWidth / 2 - boardSize / 2; 
 	int yOffset = screenHeight / 2 - boardSize / 2; 
 	int squareSize = boardSize / 8;

 	Rectangle sourceRec{ 0.f, 0.f, 2160.f / 6.f, 720.f / 2.f };
 	Rectangle destRec{};
 	Square i = 0;

 	Bitloop(pieceBitboard)
 	{
 		i = SquareOf(pieceBitboard);

 		destRec.x = xOffset + squareSize * (i%8);
 		destRec.y = yOffset + squareSize * (i/8);
 		destRec.width = squareSize;
 		destRec.height = squareSize;

 		sourceRec.x = sourceRec.width * pieceType;
 		sourceRec.y = sourceRec.height * pieceColor;

        DrawTexturePro(piecesTexture, sourceRec, destRec, Vector2{ 0,0 }, 0, WHITE);
 	}
 }