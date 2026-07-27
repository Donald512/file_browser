#pragma once

// Use C++ header for fixed-width integer types to avoid includePath issues
#include <cstdint>
#include <string>
#include <vector>


using f32 = float;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
using u8  = std::uint8_t;

enum class TriState{Unknown, True, False};

template<typename T>
struct Lazy{
    mutable T value{};
    mutable bool resolved = false;

    template<typename Fn>
    T& Get(Fn&& compute) const{
        if (!resolved){
            value = compute();
            resolved = true;
        }
        return value;
    }
    void Reset() const { // Allow re-evaluation if needed
        resolved = false;
        value = T{};
    }
};