// UCI.cpp
#include "UCI.h"
#include <iostream>
#include <sstream>

BoardState UCI::board;
MoveGenerator UCI::moveGen;
bool UCI::uciMode = false;
bool UCI::debugMode = false;

void UCI::loop() {
    Search::loadOpeningBook("./assets/baron30.bin");
    std::string line;
    while (std::getline(std::cin, line)) {
        processCommand(line);
    }
}

void UCI::processCommand(const std::string& command) {
    if (UCI::debugMode) std::cout << "Command: " << command << '\n';
    std::istringstream iss(command);
    std::string token;
    iss >> token;

    if (token == "debug")
    {
        iss >> token;
        if (token == "on")
        {
            UCI::debugMode = true;
        }
        else if (token == "off")
        {
            UCI::debugMode = false;
        }
        else
        {
            UCI::debugMode = !UCI::debugMode;
        }
    }
    if (token == "uci") {
        uciMode = true;
        std::cout << "id name ChessEngineV4\n";
        std::cout << "Bennett Friesen\n";
        std::cout << "uciok\n";
    }
    else if (token == "isready") {
        std::cout << "readyok\n";
    }
    else if (token == "position") {
        std::string fen;
        std::vector<std::string> moves;
        
        iss >> token; // Read next token after "position"
        
        if (token == "fen") {
            fen.clear();
            std::string part;
            while (iss >> part && part != "moves") {
                fen += part + " ";
            }

            if (part == "moves") {
                while (iss >> part) {
                    moves.push_back(part);
                }
            }
        } else if (token == "startpos") {
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

            if (iss >> token && token == "moves") {
                std::string move;
                while (iss >> move) {
                    moves.push_back(move);
                }
            }
        } else {
            // Invalid input, handle error if needed
            std::cerr << "Invalid position command\n";
            return;
        }

        setupPosition(fen, moves);
    }
    else if (token == "go") {
        startSearch(command.substr(3));
    }
    else if (token == "quit") {
        exit(0);
    }
}

void UCI::setupPosition(const std::string& fen, const std::vector<std::string>& moves) {

    if (fen.find("startpos") != std::string::npos) {
        board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } else {
        board.parseFEN(fen);
    }

    for (const auto& moveStr : moves) {
        MoveArr moveArr;
        int count = moveGen.generateLegalMoves(moveArr, board);
        for (int i = 0; i < count; ++i) {
            if (moveToUCI(moveArr[i]) == moveStr) {
                board.makeMove(moveArr[i]);
                break;
            }
        }
    }

	if (UCI::debugMode) printBoard(UCI::board);
}



void UCI::startSearch(const std::string& parameters) {
    Search::TimeLimit = 150; 
    int depth = 15;

	std::istringstream iss(parameters);
    std::string token;

    while (iss >> token)
    {
        if (token == "movetime")
        {
            iss >> token;
            Search::TimeLimit = std::stoi(token);
        }
        else if (token == "depth")
        {
            iss >> token;
            //depth = std::stoi(token);
        }
    }

    Move bookMove = Search::getTableMove(UCI::board);
    if (bookMove.startSquare != bookMove.endSquare)
    {
		std::cout << "bestmove " << moveToUCI(bookMove) << "\n";
        return;
    }


    

    Search::startTimer();
    Search::Timeout = false;

    Move bestMove;
    int bestScore = -1000000;
    int currentDepth = 1;

    while (currentDepth <= depth && !Search::Timeout) {
        MoveArr moves;
        int moveCount = moveGen.generateLegalMoves(moves, board);
        if (moveCount == 0) break;

        for (int i = 0; i < moveCount; ++i) {
            board.makeMove(moves[i]);
            int score = -Search::negamax(board, currentDepth-1, -1000000, 1000000, moveGen);
            board.unmakeMove();

            if (score > bestScore) {
                bestScore = score;
                bestMove = moves[i];
            }
        }
        currentDepth++;
    }

    std::cout << "bestmove " << moveToUCI(bestMove) << "\n";
}

void UCI::printBoard(const BoardState& board) {
    for (int rank = 0; rank < 8; ++rank) {
        std::string rankStr;
        for (int file = 0; file < 8; ++file) {
            const int square = rank * 8 + file;
            const Bitboard mask = 1ULL << square;
            char piece = '.';

            if (board.whiteKing & mask)        piece = 'K';
            else if (board.blackKing & mask)   piece = 'k';
            else if (board.whiteQueens & mask) piece = 'Q';
            else if (board.blackQueens & mask) piece = 'q';
            else if (board.whiteRooks & mask)  piece = 'R';
            else if (board.blackRooks & mask)  piece = 'r';
            else if (board.whiteBishops & mask)piece = 'B';
            else if (board.blackBishops & mask)piece = 'b';
            else if (board.whiteKnights & mask)piece = 'N';
            else if (board.blackKnights & mask)piece = 'n';
            else if (board.whitePawns & mask)  piece = 'P';
            else if (board.blackPawns & mask)  piece = 'p';

            rankStr += piece;
            rankStr += ' ';
        }
        std::cout << rankStr << " " << (8 - rank) << std::endl;
    }
    std::cout << "a b c d e f g h" << std::endl;
}

