#include <string>
#include <vector>
#include <cctype>
#include <cwctype>
#include "BasicTypes.h"


#include "match.h"

inline bool CharEqual(char a, char b){
    return std::tolower((u8)a) == std::tolower((u8)b);
}
inline bool WCharEqual(wchar_t a, wchar_t b){
    if (a == b) return true;
    
    if ((a | b) <= 0x7F) return (a | 0x20) == (b | 0x20) && ((a | 0x20) >= L'a' && (a | 0x20) <= L'z');

    return std::towlower((u16)a) == std::towlower((u16)b);
}

bool MatchPrefix(std::string_view query, std::string_view target) {
    if (query.empty() || query.size() > target.size()) return false;

    for (size_t i = 0; i < query.size(); i++) {
        if (!CharEqual(target[i], query[i])) return false;
    }
    return true;
}

bool MatchSubstring(std::string_view query, std::string_view target){
    if (query.empty() || query.size() > target.size()) return false;

    const size_t maxOffset = target.size() - query.size();  // upper bound for max starting positions

    for (size_t i = 0; i <= maxOffset; i++) {
        bool match = true;
        for (size_t j = 0; j < query.size(); j++) {
            if (!CharEqual(target[i + j], query[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

bool MatchFuzzySubsequence(std::string_view query, std::string_view target) {
    if (query.empty() || query.size() > target.size()) return false;

    size_t queryIdx = 0;

    for (size_t targetIdx = 0; targetIdx < target.size(); targetIdx++) {
        if (CharEqual(target[targetIdx], query[queryIdx])) {
            queryIdx++;
            if (queryIdx == query.size()) return true;
        }
    }
    return false;
}

bool MatchFuzzySubsequence(std::wstring_view query, std::wstring_view target) {
    if (query.empty() || query.size() > target.size()) return false;

    size_t queryIdx = 0;
    const size_t querySize = query.size();

    for (wchar_t targetChar : target) {
        if (WCharEqual(targetChar, query[queryIdx])) {
            if (++queryIdx == querySize) return true;
        }
    }
    return false;
}
