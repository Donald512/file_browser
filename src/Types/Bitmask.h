#pragma once

#include "BasicTypes.h"
#include "global.h"
#include <vector>


// Only IsSet currently does not check bounds, 
struct Bitmask{
private:
    std::vector<u64> blocks;
    size_t m_setBitCount = 0;
    size_t m_bitCount = 0;
public:
    class Iterator;

    Bitmask() = default;
    explicit Bitmask(size_t bitCount) {Resize(bitCount);}

    void Resize(size_t bitCount){
        size_t numBlocks = ExploraCeil(bitCount, 64);
        blocks.assign(numBlocks, 0);
        m_bitCount = bitCount;
        m_setBitCount = 0;
    }

    void Clear(){
        if (!blocks.empty()) memset(blocks.data(), 0, blocks.size() * sizeof(u64));
        m_setBitCount = 0;
    }

    void Set(size_t bitPos) {
        assert (bitPos < m_bitCount && "Bitmask set out of bounds");
        size_t blockIdx = bitPos / 64;
        u64 mask = 1ULL << (bitPos % 64);

        // Only increment if the bit was not already set
        if ((blocks[blockIdx] & mask) == 0) {
            blocks[blockIdx] |= mask;
            m_setBitCount++;
        }
    }

    bool Unset(size_t bitPos) {
        assert (bitPos < m_bitCount && "Bitmask set out of bounds");

        size_t block = bitPos / 64;
        u64 mask = (1ULL << (bitPos % 64));

        if ((blocks[block] & mask) != 0) { // Only decrement if the bit was set
            blocks[block] &= ~mask;
            m_setBitCount--;
            return true;
        }
        return false;
    }

    bool Toggle(size_t bitPos) {
        if (IsSet(bitPos)) {
            Unset(bitPos);
            return false;
        } 
        else {
            Set(bitPos);
            return true;
        }
    }

    bool IsSet(size_t bitPos){
        // actually, assert 
        assert(bitPos < m_bitCount && "Reserved wrong count, or did not reserve at all");
        size_t block = bitPos / 64;
        if (block >= blocks.size()) return false;   // Out of range 
        return (blocks[block] & (1ULL << (bitPos % 64))) != 0;
    }

    size_t BitCount() const{ return m_bitCount; }
    size_t SetBitCount() const{ return m_setBitCount; }

    void RecalculateCount(){
        m_setBitCount = 0;
        for (u64 block : blocks) {
            m_setBitCount += __popcnt64(block);
        }
    }
    
    Iterator begin() const;
    Iterator end() const;
};

class Bitmask::Iterator{
    const Bitmask* maskPtr = nullptr;
    size_t blockIdx = 0;
    u64 currentBlock = 0;
    size_t currentBitPos = 0;

    void Advance(){
        while (currentBlock == 0){  // skups completely empty blocks, eg when the u64 value is 0
            blockIdx++;
            if (blockIdx >= maskPtr->blocks.size()){
                currentBitPos = SIZE_MAX;
                return;
            }
            currentBlock = maskPtr->blocks[blockIdx];
        }

        #if defined(_MSC_VER)
            unsigned long bitIdx;
            _BitScanForward64(&bitIdx, currentBlock);
        #else
            size_t bitIdx = __builtin_ctzll(currentBlock);
        #endif

        currentBitPos = (blockIdx * 64) + bitIdx;
    }

    public:
        Iterator(const Bitmask* mask, size_t bIdx) : maskPtr(mask), blockIdx(bIdx){
            if (maskPtr && blockIdx < maskPtr->blocks.size()){
                currentBlock = maskPtr->blocks[blockIdx];
                Advance();
            }
            else currentBitPos = SIZE_MAX;
        }

        size_t operator*() const {return currentBitPos;}

        Iterator& operator++(){
            currentBlock &= (currentBlock - 1);   // Clears local copy of lowest set bit
            Advance();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return currentBitPos != other.currentBitPos;
        }
};

inline Bitmask::Iterator Bitmask::begin() const { return Iterator(this, 0); }
inline Bitmask::Iterator Bitmask::end() const { return Iterator(this, blocks.size()); }