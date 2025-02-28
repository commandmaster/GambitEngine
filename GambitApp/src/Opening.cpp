#include "Opening.h"

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
