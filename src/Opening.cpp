#include "Opening.h"

std::vector<BookEntry> loadPolyglotBook(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open book file");

    std::vector<BookEntry> entries;
    BookEntry entry;
    while (file.read(reinterpret_cast<char*>(&entry), sizeof(BookEntry))) {
        entry.key = swap64(entry.key);
        entry.move = swap16(entry.move);
        entry.weight = swap16(entry.weight);
        entry.learn = swap32(entry.learn);
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(),
        [](const BookEntry& a, const BookEntry& b) {
            return a.key < b.key;
        });
    return entries;
}

std::pair<std::vector<BookEntry>::const_iterator, std::vector<BookEntry>::const_iterator> lookupEntries(const std::vector<BookEntry>& entries, uint64_t key) {
    auto comp = [](const BookEntry& entry, uint64_t key) {
        return entry.key < key;
        };

    auto lower = std::lower_bound(entries.begin(), entries.end(), key, comp);
    auto upper = lower;
    while (upper != entries.end() && upper->key == key) {
        ++upper;
    }
    return { lower, upper };
}

uint64_t computeZobristKey(const BoardState& board)
{
    uint64_t key = 0;

    auto process = [&key](Bitboard bb, int pieceIndex)
        {
            Bitloop(bb)
            {
                Square sq = SquareOf(bb);
                key ^= Random64[pieceIndex * 64 + sq];
            }
        };

    process(board.whitePawns, 0);
    process(board.whiteKnights, 1);
    process(board.whiteBishops, 2);
    process(board.whiteRooks, 3);
    process(board.whiteQueens, 4);
    process(board.whiteKing, 5);

    process(board.blackPawns, 6);
    process(board.blackKnights, 7);
    process(board.blackBishops, 8);
    process(board.blackRooks, 9);
    process(board.blackQueens, 10);
    process(board.blackKing, 11);

    // Castling rights
    if (board.castlingRights & 1) key ^= Random64[768]; // White kingside
    if (board.castlingRights & 2) key ^= Random64[769]; // White queenside
    if (board.castlingRights & 4) key ^= Random64[770]; // Black kingside
    if (board.castlingRights & 8) key ^= Random64[771]; // Black queenside

    // En passant
    if (board.enPassant)
    {
        Square epSq = SquareOf(board.enPassant);
        int file = epSq % 8;
        key ^= Random64[772 + file];
    }

    // Side to move
    if (!board.whiteTurn) key ^= Random64[780];

    return key;
}

Move convertPolyglotMove(uint16_t polyMove, bool whiteTurn)
{
    Move move;
    move.startSquare = static_cast<uint8_t>(polyMove & 0x3F);
    move.endSquare = static_cast<uint8_t>((polyMove >> 6) & 0x3F);
    uint8_t promo = (polyMove >> 12) & 0x03;

    move.promotedPiece = Piece::NONE;
    if (promo)
    {
        uint8_t promoType;
        switch (promo)
        {
        case 0: promoType = Piece::WQ; break;
        case 1: promoType = Piece::WR; break;
        case 2: promoType = Piece::WB; break;
        case 3: promoType = Piece::WN; break;
        default: promoType = Piece::NONE;
        }
        if (!whiteTurn)
        {
            promoType = Piece::make(1, Piece::getType(promoType));
        }
        move.promotedPiece = promoType;
    }

    // Other fields will be populated when validated against legal moves
    move.piece = Piece::NONE;
    move.captureFlag = false;
    move.doublePushFlag = false;
    move.enpassantFlag = false;
    move.castlingFlag = false;

    return move;
}
