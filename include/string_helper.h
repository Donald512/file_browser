// string_helper.h

#pragma once

#include <file_browser.h>


wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount = 0, u64* outTotalNumWideChars = nullptr);
char* WideToUtf8(const wchar_t* wide);

String CreateString(const char* view_str_literal);
void DestroyString(String* target);
String CloneString(const String &source);

void AppendSubDirectory(String* path, const String* subDir);
void PopPath(String* currentDir);