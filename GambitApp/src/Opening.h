#pragma once

#include "Board.h"
#include "MoveGenerator.h"
#include "Zobrist.h"

#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <iterator>



// Used http://hgm.nubati.net/book_format.html

#pragma pack(push, 1)
struct TableEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};
#pragma pack(pop)


std::vector<TableEntry> loadPolyglotBook(const std::string &filename);

std::pair<std::vector<TableEntry>::const_iterator, std::vector<TableEntry>::const_iterator>
lookupEntries(const std::vector<TableEntry> &entries, uint64_t key);

Move convertPolyglotMove(uint16_t polyMove, bool whiteTurn);
