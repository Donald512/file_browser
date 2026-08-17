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