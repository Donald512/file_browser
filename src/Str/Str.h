#pragma once

#include "Types.h"
#include "WinFramework.h"


namespace Str{
    std::string WideToString(const wchar_t* wide);
    std::string WideToString(const std::wstring wide);
    wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount = 0, u64* outNumWideChars = nullptr);
}