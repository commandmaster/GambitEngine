#include "Board.h"

void BoardState::makeMove(const Move& move) {
    History history;
    // Save current state
    history.move = move;
    history.prevCastlingRights = castlingRights;
    history.prevEnPassant = enPassant;
    history.prevHalfmoveClock = halfmoveClock;
    history.prevFullmoveNumber = fullmoveNumber;
    history.capturedPiece = Piece::NONE;
    history.capturedSquare = 0;
    history.rookFrom = 0;
    history.rookTo = 0;

    // Handle captures
    if (move.captureFlag) {
        if (move.enpassantFlag) {
            // En passant: captured pawn is adjacent
            history.capturedSquare = whiteTurn ? move.endSquare - 8 : move.endSquare + 8;
            history.capturedPiece = whiteTurn ? Piece::BP : Piece::WP;
        } else {
            history.capturedSquare = move.endSquare;
            // Determine captured piece type
            Bitboard target = 1ULL << move.endSquare;
            if (whiteTurn) {
                if (blackPawns   & target) history.capturedPiece = Piece::BP;
                else if (blackKnights & target) history.capturedPiece = Piece::BN;
                else if (blackBishops & target) history.capturedPiece = Piece::BB;
                else if (blackRooks   & target) history.capturedPiece = Piece::BR;
                else if (blackQueens  & target) history.capturedPiece = Piece::BQ;
                else if (blackKing    & target) history.capturedPiece = Piece::BK;
            } else {
                if (whitePawns   & target) history.capturedPiece = Piece::WP;
                else if (whiteKnights & target) history.capturedPiece = Piece::WN;
                else if (whiteBishops & target) history.capturedPiece = Piece::WB;
                else if (whiteRooks   & target) history.capturedPiece = Piece::WR;
                else if (whiteQueens  & target) history.capturedPiece = Piece::WQ;
                else if (whiteKing    & target) history.capturedPiece = Piece::WK;
            }
        }
        // Remove captured piece
        switch (history.capturedPiece) {
            // Black pieces captured by white
            case Piece::BP: blackPawns   ^= (1ULL << history.capturedSquare); break;
            case Piece::BN: blackKnights ^= (1ULL << history.capturedSquare); break;
            case Piece::BB: blackBishops ^= (1ULL << history.capturedSquare); break;
            case Piece::BR: blackRooks   ^= (1ULL << history.capturedSquare); break;
            case Piece::BQ: blackQueens  ^= (1ULL << history.capturedSquare); break;
            // White pieces captured by black
            case Piece::WP: whitePawns   ^= (1ULL << history.capturedSquare); break;
            case Piece::WN: whiteKnights ^= (1ULL << history.capturedSquare); break;
            case Piece::WB: whiteBishops ^= (1ULL << history.capturedSquare); break;
            case Piece::WR: whiteRooks   ^= (1ULL << history.capturedSquare); break;
            case Piece::WQ: whiteQueens  ^= (1ULL << history.capturedSquare); break;
            default: break;
        }
        // Also update castling rights if a rook was captured from its original square.
        if (history.capturedPiece == Piece::WR) {
            if (history.capturedSquare == 63) castlingRights &= ~1; // White kingside rook captured.
            else if (history.capturedSquare == 56) castlingRights &= ~2; // White queenside rook captured.
        } else if (history.capturedPiece == Piece::BR) {
            if (history.capturedSquare == 7)  castlingRights &= ~4; // Black kingside rook captured.
            else if (history.capturedSquare == 0)  castlingRights &= ~8; // Black queenside rook captured.
        }
    }

    // Handle castling
    if (move.castlingFlag) {
        // Determine rook's movement based on destination square.
        if (whiteTurn) {
            if (move.endSquare == 6) { // White kingside castling.
                history.rookFrom = 7; history.rookTo = 5;
            } else { // White queenside castling.
                history.rookFrom = 0; history.rookTo = 3;
            }
        } else {
            if (move.endSquare == 62) { // Black kingside castling.
                history.rookFrom = 63; history.rookTo = 61;
            } else { // Black queenside castling.
                history.rookFrom = 56; history.rookTo = 59;
            }
        }
        // Move the rook.
        Bitboard rookBB = (1ULL << history.rookFrom) | (1ULL << history.rookTo);
        if (whiteTurn)
            whiteRooks ^= rookBB;
        else
            blackRooks ^= rookBB;
    }

    // Move the piece
    Bitboard fromBB = 1ULL << move.startSquare;
    Bitboard toBB   = 1ULL << move.endSquare;
    switch (move.piece) {
        // White pieces.
        case Piece::WP: whitePawns   ^= fromBB | toBB; break;
        case Piece::WN: whiteKnights ^= fromBB | toBB; break;
        case Piece::WB: whiteBishops ^= fromBB | toBB; break;
        case Piece::WR: whiteRooks   ^= fromBB | toBB; break;
        case Piece::WQ: whiteQueens  ^= fromBB | toBB; break;
        case Piece::WK: whiteKing    ^= fromBB | toBB; break;
        // Black pieces.
        case Piece::BP: blackPawns   ^= fromBB | toBB; break;
        case Piece::BN: blackKnights ^= fromBB | toBB; break;
        case Piece::BB: blackBishops ^= fromBB | toBB; break;
        case Piece::BR: blackRooks   ^= fromBB | toBB; break;
        case Piece::BQ: blackQueens  ^= fromBB | toBB; break;
        case Piece::BK: blackKing    ^= fromBB | toBB; break;
        default: break;
    }

    // Handle promotion.
    if (move.promotedPiece != Piece::NONE) {
        if (whiteTurn) {
            // Remove pawn from white pawn bitboard.
            whitePawns ^= toBB;
            switch (move.promotedPiece) {
                case Piece::WQ: whiteQueens  ^= toBB; break;
                case Piece::WR: whiteRooks   ^= toBB; break;
                case Piece::WB: whiteBishops ^= toBB; break;
                case Piece::WN: whiteKnights ^= toBB; break;
                default: break;
            }
        } else {
            blackPawns ^= toBB;
            switch (move.promotedPiece) {
                case Piece::BQ: blackQueens  ^= toBB; break;
                case Piece::BR: blackRooks   ^= toBB; break;
                case Piece::BB: blackBishops ^= toBB; break;
                case Piece::BN: blackKnights ^= toBB; break;
                default: break;
            }
        }
    }

    // Update castling rights if a king or rook moved.
    if (move.piece == Piece::WK || move.piece == Piece::BK) {
        // Moving the king forfeits all castling rights for that color.
        castlingRights &= whiteTurn ? ~0x03 : ~0x0C;
    } else if (move.piece == Piece::WR || move.piece == Piece::BR) {
        if (move.piece == Piece::WR) {
            if (move.startSquare == 63) castlingRights &= ~1; // White kingside rook moved.
            else if (move.startSquare == 56) castlingRights &= ~2; // White queenside rook moved.
        } else {
            if (move.startSquare == 7)  castlingRights &= ~4; // Black kingside rook moved.
            else if (move.startSquare == 0)  castlingRights &= ~8; // Black queenside rook moved.
        }
    }

    // Update en passant
    enPassant = move.doublePushFlag ? (1ULL << (whiteTurn ? move.endSquare + 8 : move.endSquare - 8)) : 0;

    // Update clocks
    halfmoveClock = (move.captureFlag || move.piece == Piece::WP || move.piece == Piece::BP) ? 0 : halfmoveClock + 1;
    if (!whiteTurn) fullmoveNumber++;
    whiteTurn = !whiteTurn;

    historyStack.push_back(history);
}


void BoardState::unmakeMove() {
    if (historyStack.empty()) return;
    History history = historyStack.back();
    historyStack.pop_back();
    const Move& move = history.move;

    // Revert the moving piece
    Bitboard fromBB = 1ULL << move.startSquare;
    Bitboard toBB   = 1ULL << move.endSquare;
    switch (move.piece) {
        // White pieces.
        case Piece::WP: whitePawns   ^= fromBB | toBB; break;
        case Piece::WN: whiteKnights ^= fromBB | toBB; break;
        case Piece::WB: whiteBishops ^= fromBB | toBB; break;
        case Piece::WR: whiteRooks   ^= fromBB | toBB; break;
        case Piece::WQ: whiteQueens  ^= fromBB | toBB; break;
        case Piece::WK: whiteKing    ^= fromBB | toBB; break;
        // Black pieces.
        case Piece::BP: blackPawns   ^= fromBB | toBB; break;
        case Piece::BN: blackKnights ^= fromBB | toBB; break;
        case Piece::BB: blackBishops ^= fromBB | toBB; break;
        case Piece::BR: blackRooks   ^= fromBB | toBB; break;
        case Piece::BQ: blackQueens  ^= fromBB | toBB; break;
        case Piece::BK: blackKing    ^= fromBB | toBB; break;
        default: break;
    }

    // Revert promotion if one occurred.
    if (move.promotedPiece != Piece::NONE) {
        // Determine mover's color from the piece type (white pieces have color bit 0).
        if ((move.piece & Piece::COLOR_MASK) == 0) { // White moved.
            whitePawns   ^= toBB; // Restore the pawn.
            switch (move.promotedPiece) {
                case Piece::WQ: whiteQueens  ^= toBB; break;
                case Piece::WR: whiteRooks   ^= toBB; break;
                case Piece::WB: whiteBishops ^= toBB; break;
                case Piece::WN: whiteKnights ^= toBB; break;
                default: break;
            }
        } else {
            blackPawns   ^= toBB;
            switch (move.promotedPiece) {
                case Piece::BQ: blackQueens  ^= toBB; break;
                case Piece::BR: blackRooks   ^= toBB; break;
                case Piece::BB: blackBishops ^= toBB; break;
                case Piece::BN: blackKnights ^= toBB; break;
                default: break;
            }
        }
    }

    // Restore captured piece (if any)
    if (history.capturedPiece != Piece::NONE) {
        Bitboard capturedBB = 1ULL << history.capturedSquare;
        switch (history.capturedPiece) {
            // Black pieces restored (captured by white)
            case Piece::BP: blackPawns   |= capturedBB; break;
            case Piece::BN: blackKnights |= capturedBB; break;
            case Piece::BB: blackBishops |= capturedBB; break;
            case Piece::BR: blackRooks   |= capturedBB; break;
            case Piece::BQ: blackQueens  |= capturedBB; break;
            // White pieces restored (captured by black)
            case Piece::WP: whitePawns   |= capturedBB; break;
            case Piece::WN: whiteKnights |= capturedBB; break;
            case Piece::WB: whiteBishops |= capturedBB; break;
            case Piece::WR: whiteRooks   |= capturedBB; break;
            case Piece::WQ: whiteQueens  |= capturedBB; break;
            default: break;
        }
    }

    // Revert castling: if a castling move was made, move the rook back.
    if (move.castlingFlag) {
        Bitboard rookBB = (1ULL << history.rookFrom) | (1ULL << history.rookTo);
        // Use the mover’s color (extracted from move.piece) to decide which rook bitboard to update.
        if ((move.piece & Piece::COLOR_MASK) == 0) // White moved.
            whiteRooks ^= rookBB;
        else
            blackRooks ^= rookBB;
    }

    // Restore previous state variables.
    castlingRights = history.prevCastlingRights;
    enPassant       = history.prevEnPassant;
    halfmoveClock   = history.prevHalfmoveClock;
    fullmoveNumber  = history.prevFullmoveNumber;
    whiteTurn       = !whiteTurn; // Revert turn.
}



void BoardState::parseFEN(const std::string& fen)
{
    // Reset all bitboards and state variables
    whitePawns = blackPawns = 0;
    whiteKnights = blackKnights = 0;
    whiteBishops = blackBishops = 0;
    whiteRooks = blackRooks = 0;
    whiteQueens = blackQueens = 0;
    whiteKing = blackKing = 0;
    castlingRights = 0;
    enPassant = 0;
    halfmoveClock = 0;
    fullmoveNumber = 1;

    size_t idx = 0;
    int rank = 0;  // 0 for 8th rank, 7 for 1st rank
    int file = 0;

    // Parse piece placement
    while (idx < fen.size() && fen[idx] != ' ') 
    {
        char c = fen[idx++];
        if (c == '/') 
        {
            rank++;
            file = 0;
            if (rank >= 8) {
                break;  // Invalid FEN, too many ranks
            }
        } 
        else if (isdigit(c)) 
        {
            int num = c - '0';
            file += num;
            if (file > 8) {
                break;  
            }
        } 
        else 
        {
            if (rank >= 8 || file >= 8)
            {
                break;  
            }
            int square = rank * 8 + file;
            bool isWhite = isupper(c);
            Bitboard mask = 1ULL << square;
            switch (tolower(c)) 
            {
                case 'p': isWhite ? whitePawns |= mask : blackPawns |= mask; break;
                case 'n': isWhite ? whiteKnights |= mask : blackKnights |= mask; break;
                case 'b': isWhite ? whiteBishops |= mask : blackBishops |= mask; break;
                case 'r': isWhite ? whiteRooks |= mask : blackRooks |= mask; break;
                case 'q': isWhite ? whiteQueens |= mask : blackQueens |= mask; break;
                case 'k': isWhite ? whiteKing |= mask : blackKing |= mask; break;
            }
            file++;
        }
    }

    // Parse active color
    whiteTurn = true;
    if (++idx < fen.size()) {
        whiteTurn = (fen[idx++] == 'w');
    }

    // Parse castling rights
    if (++idx < fen.size()) 
    {
        if (fen[idx] != '-') 
        {
            while (idx < fen.size() && fen[idx] != ' ') 
            {
                switch (fen[idx++])
                {
                    case 'K': castlingRights |= 1; break;
                    case 'Q': castlingRights |= 2; break;
                    case 'k': castlingRights |= 4; break;
                    case 'q': castlingRights |= 8; break;
                }
            }
        } 
        else 
        {
            idx++;
        }
    }

    // Parse en passant
    if (++idx < fen.size()) 
    {
        if (fen[idx] != '-') 
        {
            int fileEp = fen[idx] - 'a';
            int rankEp = 8 - (fen[idx + 1] - '0');  // Converts '3' to 5 (3rd rank from bottom)
            enPassant = 1ULL << (rankEp * 8 + fileEp);
            idx += 2;
        }
        else
        {
            idx++;
        }
    }

    // Parse halfmove clock
    std::string temp;
    while (++idx < fen.size() && fen[idx] != ' ') 
    {
        temp += fen[idx];
    }
    if (!temp.empty()) halfmoveClock = std::stoi(temp);

    // Parse fullmove number
    temp.clear();
    while (++idx < fen.size() && fen[idx] != ' ') 
    {
        temp += fen[idx];
    }
    if (!temp.empty()) fullmoveNumber = std::stoi(temp);
}

std::string BoardState::exportToFEN() const 
{
    std::array<char, 64> board{};
    
    auto populate = [&board](Bitboard bb, char c)
    {
        Bitloop(bb) board[SquareOf(bb)] = c;
    };

    populate(whitePawns, 'P');
    populate(blackPawns, 'p');
    populate(whiteKnights, 'N');
    populate(blackKnights, 'n');
    populate(whiteBishops, 'B');
    populate(blackBishops, 'b');
    populate(whiteRooks, 'R');
    populate(blackRooks, 'r');
    populate(whiteQueens, 'Q');
    populate(blackQueens, 'q');
    populate(whiteKing, 'K');
    populate(blackKing, 'k');

    std::string fen;
    for (int rank = 0; rank < 8; rank++) 
    {
        int empty = 0;
        for (int file = 0; file < 8; file++) 
        {
            char c = board[rank * 8 + file];
            if (c) 
            {
                if (empty) fen += std::to_string(empty);
                fen += c;
                empty = 0;
            } 
            else 
            {
                empty++;
            }
        }
        if (empty) fen += std::to_string(empty);
        if (rank < 7) fen += '/';
    }

    fen += whiteTurn ? " w " : " b ";
    if (castlingRights) 
    {
        if (castlingRights & 1) fen += 'K';
        if (castlingRights & 2) fen += 'Q';
        if (castlingRights & 4) fen += 'k';
        if (castlingRights & 8) fen += 'q';
    }
    else
    {
        fen += '-';
    }

    fen += ' ';
    if (enPassant) 
    {
        int sq = SquareOf(enPassant);
        fen += static_cast<char>('a' + (sq % 8));
        fen += static_cast<char>('0' + (8 - (sq / 8)));
    }
    else 
    {
        fen += '-';
    }

    fen += ' ' + std::to_string(halfmoveClock) + ' ' + std::to_string(fullmoveNumber);
    return fen;
}