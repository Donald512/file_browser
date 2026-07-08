// string.cpp

#include "core.h"

namespace Str{
    String Create(const char* strLiteral){
        String result;
        
        u64 len = strlen(strLiteral);
        result.capacity = 32;
        while (len >= result.capacity){
            result.capacity *= 2;
        }
        
        // this is going on the heap, not ro. data
        // So it technically is an array, technically..., not a string literal
        result.data = (char*)malloc(sizeof(char) * result.capacity);
        if (result.data == nullptr){
            printf("malloc failed for %s. capacity: %llu, length: %llu", strLiteral, result.capacity, len);
            free(result.data);
            return String{};
        }
        strcpy_s(result.data, result.capacity, strLiteral);
        result.length = len;
        return result;
    }
    
    void Free(String& target){
        if( target.data == nullptr){
            PrntErrIsNULL;
            return;
        }
        free(target.data);
        target.data = nullptr;
        target.capacity = 0;
        target.length = 0;
    }

    
    char* WideToUtf8(const wchar_t* wide){
        u64 len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
        
        if (!len){
            printf("Error in WideCharToMultiByte. Error: %lu. String: %ls\n", GetLastError(), wide);
            return nullptr;
        }
        // on heap 
        char* new_string = (char*) malloc(sizeof(char) * len);
        if (new_string == nullptr) return nullptr;
        
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, new_string, (i32) len, 0, 0);
        
        return new_string;
    }
    
    String WideToString(const wchar_t* wide){
        char* literal = WideToUtf8(wide);
        String output = Create(literal);
        free(literal);
        return output;
    }

    wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount, u64* outNumWideChars){
    // outNumWideChars != numrBytes + extraCharCount
    // an emoji could have len = 2, and 4 bytes
    // so outNumWideChars = 2 + extraCharCount, not 4 plus it, which is why the len has to be calculated in this function, not using string.length
    // if utf8 is a mixture of normal characters and emojis, dividing the length / 2 wont be accurate 
    u64 len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    
    if (!len){
        printf("Error in MultiByteToWideChar. Error: %lu. String: %s\n", GetLastError(), utf8);
        return nullptr;
    }
    
    u64 totalLen = len + extraCharCount; 
    if (outNumWideChars != nullptr){
        *outNumWideChars = totalLen;
    }
    
    wchar_t* new_string = (wchar_t*) malloc(sizeof(wchar_t) * totalLen);
    
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, new_string, (i32) len); //! must be len, not totalLen
    
    return new_string;
    }


}