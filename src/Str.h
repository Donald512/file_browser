
#pragma once
#include <string>
#include "WinFramework.h"
#include "BasicTypes.h"
#include "search.h"

namespace Str{

    inline std::string WideToString(const wchar_t* wide){
        int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
        return result;
    }
    inline std::string WideToString(std::wstring wide){
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    inline wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount, u64* outNumWideChars){
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

    inline char* WideToUtf8(const wchar_t* wide) {
        if (!wide) return nullptr;

        int sizeNeeded = ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);

        if (sizeNeeded <= 0) {
            printf("Error in WideCharToMultiByte. Error: %lu\n", ::GetLastError());
            return nullptr;
        }

        char* newString = (char*)malloc(sizeof(char) * sizeNeeded);
        if (!newString) return nullptr;

        int result = ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, newString, sizeNeeded, nullptr, nullptr);
        if (result == 0) {
            printf("Error in WideCharToMultiByte conversion. Error: %lu\n", ::GetLastError());
            free(newString);
            return nullptr;
        }

        return newString;
    }


    inline int WideToUtf8(const wchar_t* src, char* dest) {    // made this function for when i already have a location, so i dont malloc, copy to my own buffer, and free the src, eliminate the malloc twice
        if (!src) return -1;

        int sizeNeeded = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);

        if (sizeNeeded <= 0) {
            printf("Error in WideCharToMultiByte. Error: %lu\n", ::GetLastError());
            return - 1;
        }

        int result = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, dest, sizeNeeded, nullptr, nullptr);
        if (result == 0) {
            printf("Error in WideCharToMultiByte conversion. Error: %lu\n", ::GetLastError());
            return -1;
        }
        return sizeNeeded;
    }
    
    inline int GetRequiredWideToUtf8Size(const wchar_t* src){
        // Get exact UTF-8 size needed (passing nullptr for dest calculates size only)
        // Passing -1 for cchWideChar INCLUDES the null terminator in the returned size
        return ::WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
    }

    inline int WriteUtf8CharToBufferFromWide(const wchar_t* wSrc, char* dest, int destCapacity){
        if (!wSrc || !dest || destCapacity <= 0) return -1;
        int result = ::WideCharToMultiByte(CP_UTF8, 0, wSrc, -1, dest, destCapacity, nullptr, nullptr);
        if (!result) {
            printf("Error in WideCharToMultiByte conversion. Error: %lu\n", ::GetLastError());
            return -1;
        }
        return result;
    }

    std::string SanitizeWString(const wchar_t* wide){
        std::wstring wstr(wide);
        wstr.erase(std::remove_if(wstr.begin(), wstr.end(), [](wchar_t c) {
            return c == L'\r' || c == 0x200E || c == 0x200F || c == 0x202A || c == 0x202B || c == 0x202C;
        }), wstr.end());
        return WideToString(wstr.c_str());
    }

    std::wstring CleanAmpersands(const std::wstring& rawText){
        std::wstring cleanText;
        cleanText.reserve(rawText.length());

        for (size_t idx = 0; idx < rawText.length(); ++idx) {
            if (rawText[idx] == L'&') {
                if (idx + 1 < rawText.length() && rawText[idx + 1] == L'&') {
                    cleanText += L'&'; // '&&' becomes single literal '&'
                    ++idx;            // Skip second ampersand
                }
            // Single '&' is an accelerator key identifier -> skip it
            } 
            else {
                cleanText += rawText[idx];
            }
        }
        return cleanText;
    }
}

