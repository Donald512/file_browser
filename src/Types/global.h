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

inline const char* FormatFileSize(u64 size) {
    static thread_local char buf[64];
    if (size < 1024) snprintf(buf, sizeof(buf), "%llu B", size);
    else if (size < 1024*1024) snprintf(buf, sizeof(buf), "%.1f KB", size / 1024.0);
    else if (size < 1024*1024*1024) snprintf(buf, sizeof(buf), "%.1f MB", size / (1024.0*1024.0));
    else snprintf(buf, sizeof(buf), "%.2f GB", size / (1024.0*1024.0*1024.0));
    return buf;
}

inline const char* FormatFileTime(FILETIME ft) {
    static thread_local char buf[128];
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}