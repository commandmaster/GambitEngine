#ifdef PYBIND_BUILD

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "Board.h"
#include "Search.h"
#include "MoveGenerator.h"

namespace py = pybind11;

class GambitBot {
public:
    GambitBot() {
        // Initialize board to start position by default
        board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    void load_opening_book(const std::string& filename) {
        searcher.loadOpeningBook(filename);
    }

    void set_position(const std::string& fen, const std::vector<std::string>& moves) {
        if (fen == "startpos" || fen.find("startpos") != std::string::npos) {
            board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        } else {
            board.parseFEN(fen);
        }

        MoveGenerator moveGen;
        for (const auto& moveStr : moves) {
            MoveArr moveArr;
            int count;
            if (board.whiteTurn) count = moveGen.generateLegalMoves<true>(moveArr, board);
            else count = moveGen.generateLegalMoves<false>(moveArr, board);

            bool found = false;
            for (int i = 0; i < count; ++i) {
                if (moveToUCI(moveArr[i]) == moveStr) {
                    board.makeMove(moveArr[i]);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // Optional: throw error or ignore illegal moves
                // py::print("Warning: Illegal move ignored: ", moveStr);
            }
        }
    }

    std::string get_best_move(int depth, int time_limit_ms) {
        Move bestMove = searcher.findBestMove(board, depth, time_limit_ms);
        return moveToUCI(bestMove);
    }
    
    std::string get_fen() const {
        return board.exportToFEN();
    }

private:
    BoardState board;
    Searcher searcher;
};

PYBIND11_MODULE(gambit_engine, m) {
    py::class_<GambitBot>(m, "GambitBot")
        .def(py::init<>())
        .def("load_opening_book", &GambitBot::load_opening_book, "Load polyglot opening book from file path")
        .def("set_position", &GambitBot::set_position, "Set board position using FEN (or 'startpos') and a list of moves")
        .def("get_best_move", &GambitBot::get_best_move, "Find the best move given depth and time limit (ms)")
        .def("get_fen", &GambitBot::get_fen, "Get current FEN string");
}

#endif

