#include "Str.h"

namespace Str{

    std::string WideToString(const wchar_t* wide){
        int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
        return result;
    }
    std::string WideToString(std::wstring wide){
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), size, nullptr, nullptr);
        return result;
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