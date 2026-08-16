#pragma once

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
