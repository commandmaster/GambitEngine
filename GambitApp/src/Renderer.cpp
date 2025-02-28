#include "Renderer.h"

Renderer::Renderer(int screenWidth, int screenHeight)
	: screenWidth{screenWidth}, screenHeight{screenHeight}
{
    SetWindowSize(screenWidth, screenHeight);

	boardSize = std::min<int>(screenWidth, screenHeight);
	piecesTexture = LoadTexture("./assets/chessPieces.png");
	
	SetTargetFPS(60);
}

void Renderer::update(const BoardState& boardState, int8_t selectedSquare)
{
	BeginDrawing();
	ClearBackground(RAYWHITE);
	
	drawSquares();
    drawPieces(boardState, selectedSquare);
}

void Renderer::endDrawing() const
{
    EndDrawing();
}

void Renderer::drawSinglePiece(uint8_t piece, int x, int y)
{
    if (piece == Piece::NONE) return;

    x = std::max<int>(0, std::min<int>(x, GetScreenWidth()));
    y = std::max<int>(0, std::min<int>(y, GetScreenHeight()));

	Rectangle sourceRec{ 0.f, 0.f, 2160.f / 6.f, 720.f / 2.f };
 	Rectangle destRec{};

    int squareSize = boardSize / 8;

    destRec.x = x - squareSize / 2;
    destRec.y = y - squareSize / 2; 
    destRec.width = squareSize;
    destRec.height = squareSize;


    int pieceType = 0;
    int pieceColor = 0;

    constexpr int C_WHITE = 0;
 	constexpr int C_BLACK = 1;

 	constexpr int P_KING = 0;
 	constexpr int P_QUEEN = 1;
 	constexpr int P_BISHOP = 2;
 	constexpr int P_KNIGHT = 3;
 	constexpr int P_ROOK = 4;
 	constexpr int P_PAWN = 5;

    if (!(piece & Piece::COLOR_MASK))
    {
        pieceColor = C_WHITE;
		if (piece == Piece::WP) pieceType = P_PAWN;
		if (piece == Piece::WN) pieceType = P_KNIGHT;
		if (piece == Piece::WB) pieceType = P_BISHOP;
		if (piece == Piece::WR) pieceType = P_ROOK;
		if (piece == Piece::WK) pieceType = P_KING;
		if (piece == Piece::WQ) pieceType = P_QUEEN;
    }
    else
    {
        pieceColor = C_BLACK;
		if (piece == Piece::BP) pieceType = P_PAWN;
		if (piece == Piece::BN) pieceType = P_KNIGHT;
		if (piece == Piece::BB) pieceType = P_BISHOP;
		if (piece == Piece::BR) pieceType = P_ROOK;
		if (piece == Piece::BK) pieceType = P_KING;
		if (piece == Piece::BQ) pieceType = P_QUEEN;

    }

	sourceRec.x = sourceRec.width * pieceType;
	sourceRec.y = sourceRec.height * pieceColor;
    
	DrawTexturePro(piecesTexture, sourceRec, destRec, Vector2{ 0,0 }, 0, WHITE);
}

bool Renderer::shouldClose() const
{
	return WindowShouldClose();
}

void Renderer::visualizeBoard(const Bitboard& board)
{
    visualizedBoard = board;
}

void Renderer::startAnimation(int8_t startSq, int8_t endSq, int animLength)
{
    startSquare = startSq;
    endSquare = endSq;

    this->animLength = animLength;
    lerpTimer.start();
}

int8_t Renderer::getClickedSquare(int x, int y) const
{
    int xOffset = screenWidth / 2 - boardSize / 2;
	int yOffset = screenHeight / 2 - boardSize / 2;
	int squareSize = boardSize / 8;

	if (x < xOffset || x >= xOffset + boardSize || y < yOffset || y >= yOffset + boardSize)
	{
		return -1;
	}

	int col = (x - xOffset) / squareSize;
	int row = (y - yOffset) / squareSize;
	return row * 8 + col;
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
            DrawRectangle(x, y, squareSize, squareSize, Color(220, 10, 10, 110));
        }
        if (i == startSquare)
        {
            DrawRectangle(x, y, squareSize, squareSize, Color(252, 215, 3, 110));
        }

		toggle = !toggle;
		toggle = (i % 8 == 7) ? !toggle : toggle;
	}
}

void Renderer::drawPieces(const BoardState& boardState, int8_t selectedSquare) 
 {
 	constexpr int C_WHITE = 0;
 	constexpr int C_BLACK = 1;

 	constexpr int P_KING = 0;
 	constexpr int P_QUEEN = 1;
 	constexpr int P_BISHOP = 2;
 	constexpr int P_KNIGHT = 3;
 	constexpr int P_ROOK = 4;
 	constexpr int P_PAWN = 5;


 	drawPieceType(boardState.whitePawns,   P_PAWN,   C_WHITE, selectedSquare); 
 	drawPieceType(boardState.whiteRooks,   P_ROOK,   C_WHITE, selectedSquare); 
 	drawPieceType(boardState.whiteKnights,  P_KNIGHT,  C_WHITE, selectedSquare); 
    drawPieceType(boardState.whiteBishops,  P_BISHOP,  C_WHITE, selectedSquare); 
 	drawPieceType(boardState.whiteQueens,  P_QUEEN,  C_WHITE, selectedSquare); 
    drawPieceType(boardState.whiteKing,    P_KING,   C_WHITE, selectedSquare); 

 	drawPieceType(boardState.blackPawns,   P_PAWN,   C_BLACK, selectedSquare); 
 	drawPieceType(boardState.blackRooks,   P_ROOK,   C_BLACK, selectedSquare); 
 	drawPieceType(boardState.blackKnights,  P_KNIGHT,  C_BLACK, selectedSquare); 
 	drawPieceType(boardState.blackBishops,  P_BISHOP,  C_BLACK, selectedSquare); 
 	drawPieceType(boardState.blackQueens,  P_QUEEN,  C_BLACK, selectedSquare); 
 	drawPieceType(boardState.blackKing,    P_KING,   C_BLACK, selectedSquare); 
 }

void Renderer::drawPieceType(uint64_t pieceBitboard, int pieceType, int pieceColor, int8_t selectedSquare) 
 {
 	int xOffset = screenWidth / 2 - boardSize / 2; 
 	int yOffset = screenHeight / 2 - boardSize / 2; 
 	int squareSize = boardSize / 8;

 	Rectangle sourceRec{ 0.f, 0.f, 2160.f / 6.f, 720.f / 2.f };
 	Rectangle destRec{};
 	Square i = 0;

	lerpTimer.stop();
 	Bitloop(pieceBitboard)
 	{
 		i = SquareOf(pieceBitboard);

        if (i == selectedSquare)
        {
            destRec.x = GetMouseX() - squareSize / 2;
            destRec.y = GetMouseY() - squareSize / 2;
        }
        else if (i == endSquare && lerpTimer.elapsedTime<std::chrono::milliseconds>() < animLength)
        {
            float startX = xOffset + squareSize * (startSquare % 8);
            float startY = yOffset + squareSize * (startSquare / 8);
            float endX = xOffset + squareSize * (endSquare % 8);
            float endY = yOffset + squareSize * (endSquare / 8);

            destRec.x = startX + (endX - startX) * (lerpTimer.elapsedTime<std::chrono::milliseconds>() / animLength);
            destRec.y = startY + (endY - startY) * (lerpTimer.elapsedTime<std::chrono::milliseconds>() / animLength);
        }
        else
        {
			destRec.x = xOffset + squareSize * (i%8);
			destRec.y = yOffset + squareSize * (i/8);
        }
		destRec.width = squareSize;
 		destRec.height = squareSize;

 		sourceRec.x = sourceRec.width * pieceType;
 		sourceRec.y = sourceRec.height * pieceColor;

        DrawTexturePro(piecesTexture, sourceRec, destRec, Vector2{ 0,0 }, 0, WHITE);
 	}
 }




