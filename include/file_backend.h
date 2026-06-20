#pragma once

#include "file_browser.h"


DirectoryList GetDirectoryContents(const String directoryPath);
void DestroyDirectoryList(DirectoryList* directoryContents);
