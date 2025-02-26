#pragma once

#include "Board.h"

struct TTEntry
{
	static constexpr uint8_t EXACT = 0;
	static constexpr uint8_t LOWERBOUND = 1;
	static constexpr uint8_t UPPERBOUND = 2;

	uint64_t key;
	uint8_t depth;
	int score;

	uint8_t flag;
};

class TranspositionTable
{
public:
	TranspositionTable(size_t tableSizeMB)
    {
		constexpr size_t MBtoB = 1024ULL * 1024ULL;
		size_t maxBytes = tableSizeMB * MBtoB;
        size_t maxEntries = maxBytes / sizeof(TTEntry);

        tableEntries = 1ULL << (63 - __builtin_clzll(maxEntries));

        table = new TTEntry[tableEntries];
    }

	~TranspositionTable()
    {
        delete[] table;
    }

	__forceinline void store(size_t zobristKey, TTEntry entry)
    {
        size_t index = zobristKey & (tableEntries - 1);
        table[index] = entry; 
    }

    __forceinline TTEntry& retrieve(size_t zobristKey)
    {
        size_t index = zobristKey & (tableEntries - 1);
        return table[index]; 
    }
	
private:
	TTEntry* table;
	size_t tableEntries;
};
