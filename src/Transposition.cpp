#include "Transposition.h"

std::vector<TableEntry> loadPolyglotBook(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open book file");

    std::vector<TableEntry> entries;
    TableEntry entry;
    while (file.read(reinterpret_cast<char*>(&entry), sizeof(TableEntry))) {
        entry.key = swap64(entry.key);
        entry.move = swap16(entry.move);
        entry.weight = swap16(entry.weight);
        entry.learn = swap32(entry.learn);
        entries.push_back(entry);
        
    }

    std::sort(entries.begin(), entries.end(),
        [](const TableEntry& a, const TableEntry& b) {
            return a.key < b.key;
        });
    return entries;
}

std::pair<std::vector<TableEntry>::const_iterator, std::vector<TableEntry>::const_iterator> lookupEntries(const std::vector<TableEntry>& entries, uint64_t key) {
    auto comp = [](const TableEntry& entry, uint64_t key) {
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
            int row = 7 - (sq / 8); // Correct row calculation for Zobrist
            int file = sq % 8;
            key ^= Random64[pieceIndex * 64 + (row * 8 + file)];
        }

    };
    process(board.blackPawns, 0);
    process(board.whitePawns, 1);
    process(board.blackKnights, 2);
    process(board.whiteKnights, 3);
    process(board.blackBishops, 4);
    process(board.whiteBishops, 5);
    process(board.blackRooks, 6);
    process(board.whiteRooks, 7);
    process(board.blackQueens, 8);
    process(board.whiteQueens, 9);
    process(board.blackKing, 10);
    process(board.whiteKing, 11);

    // Castling rights
    if (board.castlingRights & 1) key ^= Random64[768]; // White kingside
    if (board.castlingRights & 2) key ^= Random64[769]; // White queenside
    if (board.castlingRights & 4) key ^= Random64[770]; // Black kingside
    if (board.castlingRights & 8) key ^= Random64[771]; // Black queenside

    // En passant
    if (board.enPassant)
    {
		Bitboard epRank = board.whiteTurn ? 0xff000000 : 0xff00000000;
		Bitboard notEdgeLeft = board.whiteTurn ? ~0x0101010101010101ULL : ~0x8080808080808080ULL;
        Bitboard notEdgeRight = board.whiteTurn ? ~0x8080808080808080ULL : ~0x0101010101010101ULL;

		const Bitboard& pawns = board.whiteTurn ? board.whitePawns : board.blackPawns;

        Bitboard epTarget = board.enPassant;
		if (board.whiteTurn)
		{
			epTarget <<= 8;
		}
		else
		{
            epTarget >>= 8;
		}

        Bitboard epLeftPawn = pawns & ((epTarget & ~0x0101010101010101ULL) >> 1);
		Bitboard epRightPawn = pawns & ((epTarget & ~0x8080808080808080ULL) << 1);

        if (epLeftPawn | epRightPawn)
        {
			Square epSq = SquareOf(board.enPassant);
			int file = epSq % 8;
			key ^= Random64[772 + file];

        }
    }

    // Side to move
    if (board.whiteTurn) key ^= Random64[780];

    return key;
}

Move convertPolyglotMove(uint16_t polyMove, bool whiteTurn)
{
    Move move;
    uint16_t toFile = polyMove & 0x7;
    uint16_t toRow = (polyMove & 0x38) >> 3;
    uint16_t fromFile = (polyMove & 0x1c0) >> 6;
    uint16_t fromRow = (polyMove & 0xe00) >> 9;

    uint16_t promo = (polyMove & 0x7000) >> 12;

    move.startSquare = (7 - fromRow) * 8 + fromFile;
    move.endSquare = (7 - toRow) * 8 + toFile;

    move.promotedPiece = Piece::NONE;
    if (promo)
    {
        uint8_t promoType;
        switch (promo)
        {
        case 4: promoType = Piece::WQ; break;
        case 3: promoType = Piece::WR; break;
        case 2: promoType = Piece::WB; break;
        case 1: promoType = Piece::WN; break;
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
