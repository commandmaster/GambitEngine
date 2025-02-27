#pragma once

#include <bit>
#include <cstdint>
#include <cassert>

#include "Board.h"


struct TTEntry 
{
    struct SmpData
    {
        int16_t score;
        Move move;
        uint8_t depth : 6;
        uint8_t flags : 2;

        // Ensure that EntryData is exactly 64 bits.
        static_assert(sizeof(SmpData) == sizeof(uint64_t), "SmpData must be 64 bits");

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

    uint64_t smpKey;
    SmpData smpData;
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
