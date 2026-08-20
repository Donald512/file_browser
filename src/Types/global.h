#pragma once
#include <iostream>

#define PRINTERR \
    do { \
        std::cout << "File: " << __FILE__ << "\n"; \
        std::cout << "Line: " << __LINE__ << "\n"; \
    } while(0)


template <typename T>
void shrinkVec(std::vector<T>& vec){
    if (vec.capacity() > vec.size() * 2){
        vec.shrink_to_fit();
    }
}

// Simple FNV-1a 64-bit hash function
static uint64_t HashString(const char* str) {
    uint64_t hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= static_cast<uint64_t>(*str++);
        hash *= 1099511628211ULL;
    }
    return hash;
}