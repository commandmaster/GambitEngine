#pragma once

#include <bit>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <fstream>
#include <immintrin.h>
#include <ammintrin.h>

#include "Board.h"


struct TTEntry 
{
    struct SmpData
    {
        int16_t score;
        uint8_t depth : 6;
        uint8_t flags : 2;
        Move move;

        // Convert to a 64-bit integer using std::bit_cast.
        __forceinline uint64_t to_uint64() const 
        {
            return std::bit_cast<uint64_t>(*this);
        }

        // Construct from a 64-bit integer using std::bit_cast.
       __forceinline static SmpData from_uint64(uint64_t value) 
        {
            return std::bit_cast<SmpData>(value);
        }

        // Bitwise AND
       __forceinline friend SmpData operator&(const SmpData& lhs, const SmpData& rhs) 
        {
            return from_uint64(lhs.to_uint64() & rhs.to_uint64());
        }
       __forceinline friend SmpData operator&(const SmpData& lhs, uint64_t rhs) 
        {
            return from_uint64(lhs.to_uint64() & rhs);
        }

        // Bitwise OR
       __forceinline friend SmpData operator|(const SmpData& lhs, const SmpData& rhs) 
        {
            return from_uint64(lhs.to_uint64() | rhs.to_uint64());
        }
        __forceinline friend SmpData operator|(const SmpData& lhs, uint64_t rhs) 
        {
            return from_uint64(lhs.to_uint64() | rhs);
        }

        // Bitwise XOR
       __forceinline friend SmpData operator^(const SmpData& lhs, const SmpData& rhs) 
        {
            return from_uint64(lhs.to_uint64() ^ rhs.to_uint64());
        }
       __forceinline friend SmpData operator^(const SmpData& lhs, uint64_t rhs) 
        {
            return from_uint64(lhs.to_uint64() ^ rhs);
        }

       __forceinline friend SmpData operator~(const SmpData& lhs) 
        {
            return from_uint64(~lhs.to_uint64());
        }
    };

    static constexpr uint8_t EXACT = 0;
    static constexpr uint8_t LOWERBOUND = 1;
    static constexpr uint8_t UPPERBOUND = 2;

    __forceinline static TTEntry nullEntry()
    {
        return TTEntry{ 0, { 0, 0, 0, Move{} } };
    }

    uint64_t smpKey;
    SmpData smpData;

};

class TranspositionTable
{
public:
	TranspositionTable(size_t tableSizeMB)
        : nullMove(TTEntry::nullEntry())
    {
		constexpr size_t MBtoB = 1024ULL * 1024ULL;
		size_t maxBytes = tableSizeMB * MBtoB;
        size_t maxEntries = maxBytes / sizeof(TTEntry);

        tableEntries = 1ULL << (63 - _lzcnt_u64(maxEntries));

        table = new TTEntry[tableEntries];

        std::memset(table, 0, tableEntries * sizeof(TTEntry));
    }

	~TranspositionTable()
    {
        delete[] table;
    }

	__forceinline void store(uint64_t zobristKey, TTEntry::SmpData data)
    {
        size_t index = zobristKey & (tableEntries - 1);
		TTEntry& entry = table[index];

		if (data.depth < entry.smpData.depth) {
			return;
		}

		entry.smpKey = zobristKey ^ data.to_uint64();
		entry.smpData = data;        
    }

    __forceinline TTEntry::SmpData& retrieve(uint64_t zobristKey)
    {
        size_t index = zobristKey & (tableEntries - 1);
        if ((table[index].smpKey ^ (table[index].smpData.to_uint64())) == zobristKey)
        {
			return table[index].smpData; 
        }

        return nullMove.smpData; // null data
    }
    
    void printDebugInfo() const
    {
        std::cout << "Possible Entries: " << tableEntries;
    }
    
	
private:
	TTEntry* table;
    TTEntry nullMove;
	size_t tableEntries;
};
