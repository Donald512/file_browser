#pragma once

#include <string>
#include <vector>
#include <cctype>

bool MatchPrefix(std::string_view query, std::string_view target);

bool MatchSubstring(std::string_view query, std::string_view target);

bool MatchFuzzySubsequence(std::string_view query, std::string_view target);

bool MatchFuzzySubsequence(std::wstring_view query, std::wstring_view target);
